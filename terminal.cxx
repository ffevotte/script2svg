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
    return term->opt().color.black;
  case COLOR_RED:
  case COLOR_LIGHT_RED:
    return term->opt().color.red;
  case COLOR_GREEN:
  case COLOR_LIGHT_GREEN:
    return term->opt().color.green;
  case COLOR_YELLOW:
  case COLOR_LIGHT_YELLOW:
    return term->opt().color.yellow;
  case COLOR_BLUE:
  case COLOR_LIGHT_BLUE:
    return term->opt().color.blue;
  case COLOR_MAGENTA:
  case COLOR_LIGHT_MAGENTA:
    return term->opt().color.magenta;
  case COLOR_CYAN:
  case COLOR_LIGHT_CYAN:
    return term->opt().color.cyan;
  case COLOR_LIGHT_GREY:
  case COLOR_WHITE:
    return term->opt().color.white;
  case COLOR_FOREGROUND:
    return term->opt().color.fg;
  case COLOR_BACKGROUND:
    return term->opt().color.bg;
  }

  return undefined;
}
}

bool Cell::Prop::operator!= (const Prop & other) const
{
  return fg        != other.fg
    or   bold      != other.bold
    or   underline != other.underline;
}


void AnimatedRow::update ()
{
  const std::string newState = state();
  bool upd = false;

  if (tstate_.empty()             // No previous state
      or tstate_.back().end >= 0) // Old previous state
    upd = true;
  else if (tstate_.back().state != newState) {
    // Change in state
    upd = true;
    tstate_.back().end = term_->time();
  }

  if (upd and newState != "") {
    tstate_.push_back (TimedState {
        newState, term_->time(), -1});
  }
}

void AnimatedRow::draw () const
{
  auto & out = term_->out();
  for (const auto & tstate: tstate_) {
    const double end = tstate.end > 0 ? tstate.end : term_->time();
    drawState (tstate.state,
               tstate.begin,
               end - tstate.begin);
  }
}

string RowText::state () const
{
  std::ostringstream oss;
  bool empty = true;

  const Cell::Prop defaultProp {TSM::COLOR_FOREGROUND, false, false};
  Cell::Prop currentProp = defaultProp;

  const auto & cellRow = term_->cellRow(row_);
  for (const auto & cell: cellRow) {
    if (cell.ch == ' ') {
      oss << "&#160;";
    } else {
      empty = false;

      if (cell.prop != currentProp) {
        if (currentProp != defaultProp)
          oss << SVG::propFoot().str();

        currentProp = cell.prop;
        if (currentProp != defaultProp)
          oss << SVG::propHead()
            ("$COLOR", currentProp.fg == TSM::COLOR_FOREGROUND ? "" :
             " fill='#"+TSM::color(currentProp.fg, term_)+"'")
            ("$BOLD", not currentProp.bold ? "" :
             " font-weight='bold'")
            ("$UNDERLINE", not currentProp.underline ? "" :
             " text-decoration='underline'")
            .str();
      }

      switch (cell.ch)  {
      case '<': oss << "&lt;";  break;
      case '>': oss << "&gt;";  break;
      case '&': oss << "&amp;"; break;
      default:  oss << cell.ch;
      }
    }
  }
  if (currentProp != defaultProp)
    oss << SVG::propFoot().str();

  if (empty)
    return "";

  return oss.str();
}

void RowText::drawState (const string & state,
                         double begin, double dur) const
{
  term_->out() << SVG::rowText()
    ("$X",     1)
    ("$Y",     1 + row_ * term_->opt().font.dy)
    ("$WIDTH", term_->opt().font.dx * term_->opt().columns)
    ("$TEXT",  state)
    ("$BEGIN", begin)
    ("$DUR",   dur)
    .str();
}

string RowBg::state () const
{
  std::ostringstream oss;
  int currentBg = TSM::COLOR_BACKGROUND;
  uint col0 = 0;

  auto outputBg = [&](uint col) {
    if (currentBg != TSM::COLOR_BACKGROUND)
      oss << SVG::bg()
        ("$X",     1 + col0 * term_->opt().font.dx)
        ("$Y",     1 + row_ * term_->opt().font.dy)
        ("$WIDTH", (col-col0) * term_->opt().font.dx)
        ("$DY",    term_->opt().font.dy)
        ("$COLOR", TSM::color (currentBg, term_))
        .str();
  };

  const auto & cellRow = term_->cellRow(row_);
  for (uint col = 0 ; col < term_->opt().columns ; ++col) {
    const auto & cell = cellRow[col];

    if (cell.bg != currentBg) {
      outputBg (col);
      currentBg = cell.bg;
      col0 = col;
    }
  }
  outputBg (term_->opt().columns);

  return oss.str();
}

void RowBg::drawState (const string & state,
                       double begin, double dur) const
{
  term_->out() << SVG::rowBg()
    ("$BG", state)
    ("$BEGIN", begin)
    ("$DUR", dur)
    .str();
}

Terminal::Terminal (Options & options,
                    Log::Logger & log)
  : opt_        (options),
    log_        (log),
    screen_     (log),
    vte_        (log, screen_()),
    time_       (0),
    lastUpdate_ (0)
{
  // Handle output
  if (opt().output != "-") {
    log_.write<INFO> ([&](auto&&out){
        out << "setting output to file `" << this->opt().output << "'" << std::endl;
      });
    out_.reset (new std::ofstream (opt().output), /*owner*/true);
  } else {
    out_.reset (&std::cout, /*owner*/false);
  }

  // Initialize cell matrix
  {
    log_.write<INFO> ([&](auto&&out){
        out << "setting terminal size to "
            << this->opt().columns << "x" << this->opt().rows << std::endl;
      });

    tsm_screen_resize(screen_(), opt().columns, opt().rows);

    cell_.resize(opt().rows);
    for (uint row=0 ; row<opt().rows ; ++row) {
      cell_[row].resize(opt().columns);
    }
  }

  // Initialize row vectors
  rowText_.resize (opt().rows);
  rowBg_.resize (opt().rows);
  for (uint row=0 ; row<opt().rows ; ++row) {
    rowText_[row].init (this, row);
    rowBg_[row].init   (this, row);
  }

  const int width = 1 + opt().font.dx*(0.5+opt().columns);

  const int height = 1 + opt().font.dy*(0.5+opt().rows) + opt().progress.height;

  out() << SVG::header()
    ("$FONT", opt().font.family)
    ("$SIZE", opt().font.size)
    ("$FG",   opt().color.fg)
    ("$WIDTH",  width + opt().font.size + 1)
    ("$HEIGHT", height + 1)
    .str();

  if (opt().ad.text != "") {
    out() << SVG::advertisement()
      ("$X",    width)
      ("$Y",    height)
      ("$SIZE", int (opt().font.size * 0.75))
      ("$URL",  opt().ad.url)
      ("$TEXT", opt().ad.text)
      .str();
  }
}

Terminal::~Terminal ()
{
  // Progress bar
  out() << SVG::progress()
    ("$X0",    1)
    ("$Y0",    1 + opt().font.dy * (opt().rows + 0.5))
    ("$DX",    opt().font.dx * opt().columns)
    ("$DY",    opt().progress.height)
    ("$TIME",  time_)
    ("$COLOR", opt().progress.color)
    .str();
  time_ += 1;

  // Background
  out() << SVG::bgHead()
    ("$WIDTH",  opt().font.dx * opt().columns + 2)
    ("$HEIGHT", opt().font.dy * opt().rows + 2)
    ("$BG",     opt().color.bg)
    .str();
  for (const auto & row: rowBg_) {row.draw();}

  // Text
  out() << SVG::textHead().str();
  for (const auto & row: rowText_) {row.draw();}

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

  for (auto & row : rowText_) {row.update();}
  for (auto & row : rowBg_)   {row.update();}
}

int Terminal::update (struct tsm_screen *screen, uint32_t id,
                      const uint32_t *ch, size_t len, unsigned int cwidth,
                      unsigned int col, unsigned int row,
                      const struct tsm_screen_attr *attr,
                      tsm_age_t age, void *data)
{
  Terminal * term = (Terminal*)data;

  if (row >= term->opt().rows)
    return 1;
  if (col >= term->opt().columns)
    return 1;

  auto & cell = term->cell_[row][col];

  cell.prop.fg = attr->fccode;
  cell.bg = attr->bccode;

  if (attr->inverse) {
    std::swap (cell.prop.fg, cell.bg);
  }

  cell.prop.bold = attr->bold;
  cell.prop.underline = attr->underline;

  if (len) {
    cell.ch = static_cast<char>(*ch);
  } else {
    cell.ch = ' ';
  }

  return 0;
}
