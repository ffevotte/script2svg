#include "terminal.hxx"
#include <fstream>
#include <iomanip>

Cell::State::State ()
  : ch (' ')
{}

bool Cell::State::operator!= (const State & other) const
{
  return ch != other.ch;
}

void Cell::pos (unsigned int x, unsigned int y)
{
  x_ = x;
  y_ = y;
}

void Cell::draw (std::ostream & out, double end)
{
  if (state_.ch != ' ') {
    out << "<text style=\"font-family: Courier\""
        << " x=\"" << 20+x_*10 << "\""
        << " y=\"" << 20+y_*14 << "\""
        << " visibility=\"hidden\">";

    switch (state_.ch) {
    case '<': out << "&lt;"; break;
    case '>': out << "&gt;"; break;
    default:  out << state_.ch;
    }

    out << std::endl
        << " <set attributeName=\"visibility\" attributeType=\"CSS\" to=\"visible\""
        << " begin=\"start.end+" << begin_ << "\""
        << " dur=\"" << end-begin_ << "\""
        << "/>" << std::endl;

    out << "</text>" << std::endl;
  }
}


void Cell::update (std::ostream & out, double time, const State & state)
{
  if (state != state_) {
    draw(out, time);
    begin_ = time;
    state_ = state;
  }
}

Terminal::Terminal (Log::Logger & log, std::ostream & out)
  : log_    (log),
    out_    (out),
    screen_ (log),
    vte_    (log, screen_()),
    time_   (0)
{
  tsm_screen_resize(screen_(), 80, 54);

  cell_.resize(80);
  for (uint col=0 ; col<cell_.size() ; ++col) {
    cell_[col].resize(54);
    for (uint row=0 ; row<cell_[col].size() ; ++row) {
      cell_[col][row].pos(col,row);
    }
  }

  out_ << "<svg xmlns=\"http://www.w3.org/2000/svg\""
       << " xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << std::endl
       << "<text>"
       << "<set id=\"start\" attributeName=\"visibility\" attributeType=\"CSS\" to=\"visible\""
       << " begin=\"0; progress.end+1\" dur=\"0.001\"/>"
       << "</text>" << std::endl;
}

Terminal::~Terminal ()
{
  for (uint col = 0 ; col<cell_.size() ; col++) {
    for (uint row = 0 ; row<cell_[col].size() ; ++row) {
      cell_[col][row].draw(out_, time_+1);
    }
  }
  out_ << "<rect x=\"20\" y=\"770\" width=\"800\" height=\"5\" style=\"stroke:#0000cc; fill:none\" />" << std::endl
       << "<rect x=\"20\" y=\"770\" height=\"5\" style=\"fill:#0000cc;\">"
       << "<animate id=\"progress\" attributeName=\"width\" attributeType=\"XML\""
       << " from=\"0\"  to=\"800\" fill=\"freeze\""
       << " begin=\"start.end\" dur=\"" << time_ << "\" />"
       << "</rect>" << std::endl
       << "</svg>";
}

void Terminal::play (const std::string & scriptPath, const std::string timingPath)
{
  std::ifstream script (scriptPath, std::ifstream::in);
  { // Discard first line
    std::string discard;
    std::getline(script, discard);
  }

  constexpr int bufferSize {30};
  std::unique_ptr<char[]> bufGuard {new char [bufferSize]};
  char * buffer {bufGuard.get()};

  std::ifstream timing (timingPath, std::ifstream::in);
  double lastUpdate = 0;
  double shouldUpdate = -1;

  while (true) {
    double delay;
    int nb;
    timing >> delay >> nb;
    if (timing.good()) {
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
        script.read(buffer, n);
        buffer[n] = 0;
        tsm_vte_input (vte_(), buffer, n);

        log_.write<Log::DEBUG> ([&buffer, &n, &delay, this](auto&&out){
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
                out << "\\" << std::setw(3) << std::setfill('0') << std::oct << (unsigned int)(*c);
              else
                out << *c;
            }
            out << "]" << std::endl;
          });

        delay = -1;
        nb -= n;
      }
    } else {
      update();
      break;
    }
  }
}

void Terminal::update () {
  log_.write<Log::DEBUG> ([&](auto&&out){
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
  cell_[col][row].update (out_, time_, state);
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
