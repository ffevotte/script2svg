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
     " </text>\n");
}

Template footer ()
{
  return Template
    ("</svg>\n");
}


Template progress ()
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
     " </rect>\n");
}


Template cursorHead ()
{
  return Template
    (" <!-- Cursor -->\n"
     " <rect width='$DX' height='$DY'"
     "  style='stroke:#000000; fill:none;'>\n");
}

Template cursorFoot ()
{
  return Template
    (" </rect>\n");
}

Template cursorPos ()
{
  return Template
    ("  <set attributeName='x' attributeType='XML' to='$X'"
     "   begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     "  <set attributeName='y' attributeType='XML' to='$Y'"
     "   begin='start.begin+$BEGIN' dur='$DUR'/>\n");
}


Template termHead ()
{
  return Template
    (" <!-- Cell matrix -->\n"
     " <text dominant-baseline='text-before-edge'>\n");
}

Template termFoot ()
{
  return Template
    (" </text>\n");
}


Template cell ()
{
  return Template
    ("  <tspan x='$X' y='$Y' visibility='hidden' style='fill:#$FG'>$CHAR\n"
     "   <set attributeName='visibility' attributeType='CSS' to='visible'"
     "    begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     "  </tspan>\n");
}

// Imported from "tsm_vte.c"
enum vte_color {
	COLOR_BLACK,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_LIGHT_GREY,
	COLOR_DARK_GREY,
	COLOR_LIGHT_RED,
	COLOR_LIGHT_GREEN,
	COLOR_LIGHT_YELLOW,
	COLOR_LIGHT_BLUE,
	COLOR_LIGHT_MAGENTA,
	COLOR_LIGHT_CYAN,
	COLOR_WHITE,
	COLOR_FOREGROUND,
	COLOR_BACKGROUND,
	COLOR_NUM
};

const std::string & color (int code, const Terminal * term) {
  using std::string;

  switch (code) {
  case COLOR_BLACK:
  case COLOR_DARK_GREY:
    return term->vm<string>("color.black");
  case COLOR_RED:
  case COLOR_LIGHT_RED:
    return term->vm<string>("color.red");
  case COLOR_GREEN:
  case COLOR_LIGHT_GREEN:
    return term->vm<string>("color.green");
  case COLOR_YELLOW:
  case COLOR_LIGHT_YELLOW:
    return term->vm<string>("color.yellow");
  case COLOR_BLUE:
  case COLOR_LIGHT_BLUE:
    return term->vm<string>("color.blue");
  case COLOR_MAGENTA:
  case COLOR_LIGHT_MAGENTA:
    return term->vm<string>("color.magenta");
  case COLOR_CYAN:
  case COLOR_LIGHT_CYAN:
    return term->vm<string>("color.cyan");
  case COLOR_LIGHT_GREY:
  case COLOR_WHITE:
    return term->vm<string>("color.white");
  case COLOR_FOREGROUND:
    return term->vm<string>("color.fg");
  case COLOR_BACKGROUND:
    return term->vm<string>("color.bg");
  }
}

}


void Cursor::update (const Position & pos)
{
  TimedPosition tpos {pos, term_->time(), -1};

  if (tpos_.empty()) {
    tpos_.push_back (tpos);
    return;
  }

  if (tpos_.back().pos != pos) {
    tpos_.back().end = tpos.begin;
    tpos_.push_back (tpos);
  }
}

void Cursor::draw () const
{
  std::ostream & out = term_->out();
  out << SVG::cursorHead()
    ("$DX", term_->vm<int>("dx"))
    ("$DY", term_->vm<int>("dy"))
    .str();

  for (auto tpos: tpos_) {
    const double end = tpos.end > 0 ? tpos.end : term_->time();
    out << SVG::cursorPos()
      ("$X",     20 + term_->vm<int>("dx") * tpos.pos.x)
      ("$Y",     20 + term_->vm<int>("dy") * tpos.pos.y)
      ("$BEGIN", tpos.begin)
      ("$DUR",   end - tpos.begin)
      .str();
  }

  out << SVG::cursorFoot().str();
}

Cell::State::State ()
  : ch (' ')
{}

bool Cell::State::operator!= (const State & other) const
{
  return ch != other.ch
    or   fg != other.fg;
}

void Cell::term (const Terminal * term)
{
  term_ = term;
}

void Cell::update (const State & state)
{
  bool upd = false;

  if (tstate_.empty()) {
    // No previous state
    upd = true;
  } else if (tstate_.back().end >= 0) {
    // Old previous state
    upd = true;
  } else if (tstate_.back().state != state) {
    // Change in state
    upd = true;
    tstate_.back().end = term_->time();
  }

  if (upd and state.ch != ' ') {
    TimedState tstate {state, term_->time(), -1};
    tstate_.push_back (tstate);
  }
}

void Cell::draw (uint col, uint row) const
{
  if (tstate_.empty())
    return;

  std::ostream & out = term_->out();

  for (auto tstate: tstate_) {
    std::string ch {tstate.state.ch};

    // Convert special chars to XML entities
    switch (tstate.state.ch) {
    case '<': ch = "&lt;";  break;
    case '>': ch = "&gt;";  break;
    case '&': ch = "&amp;"; break;
    }

    const double end = tstate.end>0 ? tstate.end : term_->time();

    out << SVG::cell()
      ("$X",     20 + col * term_->vm<int>("dx"))
      ("$Y",     20 + row * term_->vm<int>("dy"))
      ("$CHAR",  ch)
      ("$FG",    SVG::color(tstate.state.fg, term_))
      ("$BEGIN", tstate.begin)
      ("$DUR",   end-tstate.begin)
      .str();
  }
}

Terminal::Terminal (const po::variables_map & vm,
                    Log::Logger & log)
  : vm_         (vm),
    log_        (log),
    screen_     (log),
    vte_        (log, screen_()),
    time_       (0),
    lastUpdate_ (0)
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

  // Initialize cell matrix
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

  // Initialize cursor
  cursor_.term (this);

  out() << SVG::header()
    ("$FONT", this->vm<std::string>("font"))
    ("$SIZE", this->vm<std::string>("size"))
    .str();
}

Terminal::~Terminal ()
{
  // Progress bar
  out() << SVG::progress()
    ("$X0",    20)
    ("$Y0",    vm<int>("dy") * (vm<int>("rows") + 0.5) + 20)
    ("$DX",    vm<int>("dx") * vm<int>("columns"))
    ("$DY",    vm<std::string>("progress.height"))
    ("$TIME",  time_)
    ("$COLOR", vm<std::string>("progress.color"))
    .str();
  time_ += 1;

  // Cursor positions
  cursor_.draw();

  // Cells matrix
  out() << SVG::termHead().str();
  for (uint row = 0 ; row<cell_[0].size() ; ++row) {
    for (uint col = 0 ; col<cell_.size() ; col++) {
      cell_[col][row].draw(col, row);
    }
  }
  out() << SVG::termFoot().str();

  // SVG footer
  out() << SVG::footer().str();
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
          and time_ > lastUpdate_ + 0.01) {
        shouldUpdate = time_;
      }

      if (shouldUpdate > 0
          and time_ + delay > shouldUpdate + 0.01) {
        update();
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
  cursor_.update (Cursor::Position {tsm_screen_get_cursor_x (screen_()),
        tsm_screen_get_cursor_y (screen_())});
  lastUpdate_ = time_;
}

void Terminal::update (uint col, uint row, const Cell::State & state)
{
  if (col >= cell_.size())      return;
  if (row >= cell_[col].size()) return;
  cell_[col][row].update (state);
}


int Terminal::update (struct tsm_screen *screen, uint32_t id,
                      const uint32_t *ch, size_t len, unsigned int cwidth,
                      unsigned int posx, unsigned int posy,
                      const struct tsm_screen_attr *attr,
                      tsm_age_t age, void *data)
{
  Terminal * term = (Terminal*)data;

  Cell::State state;

  state.fg = attr->fccode;

  if (len) {
    state.ch = static_cast<char>(*ch);
  }
  term->update (posx, posy, state);

  return 0;
}
