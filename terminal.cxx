#include "svg.hxx"
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
using std::string;

namespace TSM {

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

const string & color (int code, const Terminal * term) {
  static const string undefined = "xxxxxx";

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

  return undefined;
}
}

Cell::Text::Text ()
  : ch (' ')
{}

bool Cell::Text::operator!= (const Text & other) const
{
  return ch        != other.ch
    or   fg        != other.fg
    or   bold      != other.bold
    or   underline != other.underline;
}

void Cell::term (const Terminal * term)
{
  term_ = term;
}

void Cell::updateText (const Text & text)
{
  bool upd = false;

  if (ttext_.empty()) {
    // No previous text
    upd = true;
  } else if (ttext_.back().end >= 0) {
    // Old previous text
    upd = true;
  } else if (ttext_.back().text != text) {
    // Change in text
    upd = true;
    ttext_.back().end = term_->time();
  }

  if (upd and text.ch != ' ') {
    TimedText ttext {text, term_->time(), -1};
    ttext_.push_back (ttext);
  }
}

void Cell::drawText (uint col, uint row) const
{
  if (ttext_.empty())
    return;

  std::ostream & out = term_->out();

  for (auto ttext: ttext_) {
    string ch {ttext.text.ch};

    // Convert special chars to XML entities
    switch (ttext.text.ch) {
    case '<': ch = "&lt;";  break;
    case '>': ch = "&gt;";  break;
    case '&': ch = "&amp;"; break;
    }

    const double end = ttext.end>0 ? ttext.end : term_->time();

    out << SVG::textCell()
      ("$X",     20 + col * term_->vm<int>("dx"))
      ("$Y",     20 + row * term_->vm<int>("dy"))
      ("$CHAR",  ch)
      ("$COLOR", ttext.text.fg != TSM::COLOR_FOREGROUND ?
       (" fill='#" + TSM::color(ttext.text.fg, term_) + "'") : "")
      ("$BOLD",  ttext.text.bold ? " font-weight='bold'" : "")
      ("$UNDER", ttext.text.underline ? " text-decoration='underline'" : "")
      ("$BEGIN", ttext.begin)
      ("$DUR",   end-ttext.begin)
      .str();
  }
}

void Cell::updateBg (int bg)
{
  bool upd = false;

  if (tbg_.empty()) {
    // No previous bg
    upd = true;
  } else if (tbg_.back().end >= 0) {
    // Old previous bg
    upd = true;
  } else if (tbg_.back().bg != bg) {
    // Change in bg
    upd = true;
    tbg_.back().end = term_->time();
  }

  if (upd and bg != TSM::COLOR_BACKGROUND) {
    TimedBg tbg {bg, term_->time(), -1};
    tbg_.push_back (tbg);
  }
}

void Cell::drawBg (uint col, uint row) const
{
  if (tbg_.empty())
    return;

  std::ostream & out = term_->out();

  for (auto tbg: tbg_) {
    const double end = tbg.end>0 ? tbg.end : term_->time();

    out << SVG::bgCell()
      ("$X",     20 + col * term_->vm<int>("dx"))
      ("$Y",     20 + row * term_->vm<int>("dy"))
      ("$DX",    term_->vm<int>("dx"))
      ("$DY",    term_->vm<int>("dy"))
      ("$COLOR", TSM::color(tbg.bg, term_))
      ("$BEGIN", tbg.begin)
      ("$DUR",   end-tbg.begin)
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
    string outputName = this->vm<string>("output");
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

  out() << SVG::header()
    ("$FONT", this->vm<string>("font"))
    ("$SIZE", this->vm<string>("size"))
    .str();
}

Terminal::~Terminal ()
{
  // Progress bar
  out() << SVG::progress()
    ("$X0",    20)
    ("$Y0",    vm<int>("dy") * (vm<int>("rows") + 0.5) + 20)
    ("$DX",    vm<int>("dx") * vm<int>("columns"))
    ("$DY",    vm<string>("progress.height"))
    ("$TIME",  time_)
    ("$COLOR", vm<string>("progress.color"))
    .str();
  time_ += 1;

  // Background
  out() << SVG::bgHead()
    ("$X0",     20-1)
    ("$Y0",     20-1)
    ("$WIDTH",  vm<int>("dx") * vm<int>("columns") + 2)
    ("$HEIGHT", vm<int>("dy") * vm<int>("rows") + 2)
    ("$BG",     vm<string>("color.bg"))
    .str();
  for (uint row = 0 ; row<cell_[0].size() ; ++row) {
    for (uint col = 0 ; col<cell_.size() ; col++) {
      cell_[col][row].drawBg (col, row);
    }
  }

  // Text
  out() << SVG::textHead()
    ("$FG", vm<string>("color.fg"))
    .str();
  for (uint row = 0 ; row<cell_[0].size() ; ++row) {
    for (uint col = 0 ; col<cell_.size() ; col++) {
      cell_[col][row].drawText (col, row);
    }
  }
  out() << SVG::textFoot().str();

  // SVG footer
  out() << SVG::footer().str();
}

void Terminal::play (const string & scriptPath, const string timingPath)
{
  std::ifstream script {scriptPath, std::ifstream::in};
  if (script.fail()) {
    std::ostringstream oss;
    oss << "could not read script file `" << scriptPath << "'";
    throw std::runtime_error {oss.str()};
  }

  { // Discard first line
    string discard;
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
          throw std::runtime_error
            ("premature end of script file; stopping processing here.");
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
      throw std::runtime_error
        ("could not parse timing file; stopping processing here");
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
  lastUpdate_ = time_;
}

void Terminal::update (uint col, uint row, const Cell::Text & text, int bg)
{
  if (col >= cell_.size())      return;
  if (row >= cell_[col].size()) return;
  cell_[col][row].updateText (text);
  cell_[col][row].updateBg (bg);
}


int Terminal::update (struct tsm_screen *screen, uint32_t id,
                      const uint32_t *ch, size_t len, unsigned int cwidth,
                      unsigned int posx, unsigned int posy,
                      const struct tsm_screen_attr *attr,
                      tsm_age_t age, void *data)
{
  Terminal * term = (Terminal*)data;

  Cell::Text text;
  int bg;

  text.fg = attr->fccode;
  bg = attr->bccode;

  if (attr->inverse) {
    std::swap (text.fg, bg);
  }

  text.bold = attr->bold;
  text.underline = attr->underline;

  if (len) {
    text.ch = static_cast<char>(*ch);
  }
  term->update (posx, posy, text, bg);

  return 0;
}
