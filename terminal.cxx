#include "terminal.hxx"
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <sstream>

using Log::ERROR;
using Log::WARNING;
using Log::NOTICE;
using Log::INFO;
using Log::DEBUG;

namespace po = boost::program_options;

namespace SVG
{
class Template
{
public:
  Template (std::string str)
    : str_(str)
  {}

  template <typename T>
  Template & operator() (std::string placeholder, T value)
  {
    std::ostringstream oss;
    oss << value;

    size_t pos = 0;
    while ((pos = str_.find (placeholder, pos)) != std::string::npos) {
      str_.replace (pos, placeholder.size(), oss.str());
    }
    return *this;
  }

  const std::string & str () const
  {
    return str_;
  }

private:
  std::string str_;
};

Template header ()
{
  return Template
    ("<svg xmlns='http://www.w3.org/2000/svg'"
     " xmlns:xlink='http://www.w3.org/1999/xlink'\n"
     " font-family='$FONT' font-size='$SIZE'>\n"
     " <!-- Cycling -->\n"
     " <text>\n"
     "  <set id='start' attributeName='visibility' attributeType='CSS' to='visible'"
     "   begin='0; progress.end+1' dur='0'/>\n"
     " </text>\n"
     " <!-- Terminal -->\n");
}

Template cell ()
{
  return Template
    (" <text x='$X' y='$Y' visibility='hidden'>$CHAR\n"
     "  <set attributeName='visibility' attributeType='CSS' to='visible'"
     "   begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     " </text>\n");
}

Template footer ()
{
  return Template
    (" <!-- Progress bar -->\n"
     " <!--   border -->\n"
     " <rect x='$X0' y='$Y0' width='$DX' height='$DY'"
     "  style='stroke:#$COLOR; fill:none' />\n"
     " <!--   filling -->\n"
     " <rect x='$X0' y='$Y0' height='$DY' style='fill:#$COLOR;'>\n"
     "  <animate id='progress' attributeName='width' attributeType='XML'"
     "   from='0' to='$DX' fill='freeze'"
     "   begin='start.begin' dur='$TIME' />\n"
     " </rect>\n"
     "</svg>\n");
}
}

Cell::State::State ()
  : ch (' ')
{}

bool Cell::State::operator!= (const State & other) const
{
  return ch != other.ch;
}

void Cell::term (const Terminal * term)
{
  term_ = term;
}

void Cell::draw (int x, int y, double end)
{
  std::ostream & out = term_->out();
  if (state_.ch != ' ') {
    std::string ch {state_.ch};

    // Convert special chars to XML entities
    switch (state_.ch) {
    case '<': ch = "&lt;"; break;
    case '>': ch = "&gt;"; break;
    }

    out << SVG::cell()
      ("$X",     20 + x * term_->vm<int>("dx"))
      ("$Y",     20 + y * term_->vm<int>("dy"))
      ("$CHAR",  ch)
      ("$BEGIN", begin_)
      ("$DUR",   end-begin_)
      .str();
  }
}


void Cell::update (int x, int y, const State & state)
{
  if (state != state_) {
    draw(x, y, term_->time());
    begin_ = term_->time();
    state_ = state;
  }
}

Terminal::Terminal (const po::variables_map & vm,
                    Log::Logger & log)
  : vm_     (vm),
    log_    (log),
    screen_ (log),
    vte_    (log, screen_()),
    time_   (0)
{
  // Handle output
  if (vm_.count ("output")) {
    std::string outputName = this->vm<std::string>("output");
    log_.write<INFO> ([&](auto&&out){
        out << "setting output to file `" << outputName << "'" << std::endl;
      });
    out_.reset (new std::ofstream (outputName), /*owner*/true);
  } else {
    out_.reset (&std::cout, /*owner*/false);
  }

  // Terminal size
  {
    int nCols = this->vm<int>("columns");
    if (nCols == 0) {
      const char * var = std::getenv("COLUMNS");
      if (var) {
        std::istringstream iss {var};
        iss >> nCols;
      } else {
        throw std::runtime_error
          ("empty COLUMNS environment variable; "
           "please provide the `--columns' command-line option");
      }
    }

    int nRows = this->vm<int>("rows");
    if (nRows == 0) {
      const char * var = std::getenv("LINES");
      if (var) {
        std::istringstream iss {var};
        iss >> nRows;
      } else {
        throw std::runtime_error
          ("empty LINES environment variable; "
           "please provide the `--rows' command-line option");
      }
    }

    log_.write<INFO> ([&](auto&&out){
        out << "setting terminal size to "
            << nCols << "x" << nRows << std::endl;
      });

    tsm_screen_resize(screen_(), nCols, nRows);

    cell_.resize(nCols);
    for (uint col=0 ; col<cell_.size() ; ++col) {
      cell_[col].resize(nRows);
      for (uint row=0 ; row<cell_[col].size() ; ++row) {
        cell_[col][row].term(this);
      }
    }
  }

  out() << SVG::header()
    ("$FONT", this->vm<std::string>("font"))
    ("$SIZE", this->vm<std::string>("size"))
    .str();
}

Terminal::~Terminal ()
{
  for (uint col = 0 ; col<cell_.size() ; col++) {
    for (uint row = 0 ; row<cell_[col].size() ; ++row) {
      cell_[col][row].draw(col, row, time_+1);
    }
  }

  out() << SVG::footer()
    ("$X0",    20)
    ("$Y0",    vm<int>("dy") * vm<int>("rows") + 20)
    ("$DX",    vm<int>("dx") * vm<int>("columns"))
    ("$DY",    vm<std::string>("progress.height"))
    ("$TIME",  time_)
    ("$COLOR", vm<std::string>("progress.color"))
    .str();
}

void Terminal::play (const std::string & scriptPath, const std::string timingPath)
{
  std::ifstream script {scriptPath, std::ifstream::in};
  if (script.fail()) {
    std::ostringstream oss;
    oss << "could not read script file `" << scriptPath << "'";
    throw std::runtime_error {oss.str()};
  }

  { // Discard first line
    std::string discard;
    std::getline(script, discard);
  }

  std::ifstream timing {timingPath, std::ifstream::in};
  if (timing.fail()) {
    std::ostringstream oss;
    oss << "could not read timing file `" << timingPath << "'";
    throw std::runtime_error {oss.str()};
  }


  constexpr int bufferSize {30};
  std::unique_ptr<char[]> bufGuard {new char [bufferSize]};
  char * buffer {bufGuard.get()};

  double lastUpdate = 0;
  double shouldUpdate = -1;

  { // Discard the first delay
    //
    // I don't understand why there is a shift of one line for the "delay"
    // column...
    double discard;
    timing >> discard;
  }

  while (true) {
    int nb;
    timing >> nb;
    if (timing.good()) {
      // The delay is taken to be 0 if it can not be read (happens for the last
      // line of the timing file, again because of this unexplained shift)
      double delay = 0;
      timing >> delay;

      if (shouldUpdate < 0
          and time_ > lastUpdate + 0.01) {
        shouldUpdate = time_;
      }

      if (shouldUpdate > 0
          and time_ + delay > shouldUpdate + 0.01) {
        update();
        lastUpdate = time_;
        shouldUpdate = -1;
      }

      time_ += delay;

      while (nb>0) {
        const int n = std::min(nb, bufferSize-1);
        script.read(buffer, n); buffer[n] = 0;
        if (script.fail()) {
          throw std::runtime_error ("premature end of script file;"
                                    " stopping processing here.");
        }

        tsm_vte_input (vte_(), buffer, n);

        log_.write<DEBUG> ([&buffer, &n, &delay, this](auto&&out){
            out << "[term input] " << std::setfill(' ');
            if (delay >= 0)
              out << std::setprecision(5) << std::fixed
                  << std::setw(7) << delay << "  "
                  << std::setw(9) << time_;
            else
              out << std::setw(7) << "" << "  "
                  << std::setw(9) << "";
            out << " [";
            for (char * c = buffer ; c<buffer+n ; ++c) {
              if (*c < ' ')
                out << "\\" << std::setw(3) << std::setfill('0') << std::oct
                    << (unsigned int)(*c);
              else
                out << *c;
            }
            out << "]" << std::endl;
          });

        delay = -1;
        nb -= n;
      }
    } else if (timing.eof()) {
      update();
      break;
    } else {
      throw std::runtime_error ("could not parse timing file;"
                                " stopping processing here");
    }
  }
}

void Terminal::update ()
{
  log_.write<DEBUG> ([&](auto&&out){
      out << "[term update]-------- "
          << std::setfill(' ') << std::setw(9)
          << time_ << std::endl;
    });
  tsm_screen_draw (screen_(), update, this);
}

void Terminal::update (uint col, uint row, const Cell::State & state)
{
  if (col >= cell_.size())      return;
  if (row >= cell_[col].size()) return;
  cell_[col][row].update (col, row, state);
}


int Terminal::update (struct tsm_screen *screen, uint32_t id,
                      const uint32_t *ch, size_t len, unsigned int cwidth,
                      unsigned int posx, unsigned int posy,
                      const struct tsm_screen_attr *attr,
                      tsm_age_t age, void *data)
{
  Terminal * term = (Terminal*)data;

  Cell::State state;
  if (len) {
    state.ch = static_cast<char>(*ch);
  }
  term->update (posx, posy, state);

  return 0;
}
