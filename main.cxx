#include "libtsm.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>


static const char *sev2str_table[] = {
  "FATAL",
  "ALERT",
  "CRITICAL",
  "ERROR",
  "WARNING",
  "NOTICE",
  "INFO",
  "DEBUG"
};

static const char *sev2str(unsigned int sev)
{
  if (sev > 7)
    return "DEBUG";

  return sev2str_table[sev];
}

static void log_tsm(void *data, const char *file, int line, const char *fn,
		    const char *subs, unsigned int sev, const char *format,
		    va_list args)
{
  fprintf(stderr, "%s: %s: ", sev2str(sev), subs);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
}

static void term_write_cb(struct tsm_vte *vte, const char *u8, size_t len,
			  void *data)
{
  fprintf (stderr, "DEBUG: [%s]\n", u8);
}


static int draw_cell(struct tsm_screen *screen, uint32_t id,
                     const uint32_t *ch, size_t len, unsigned int cwidth,
                     unsigned int posx, unsigned int posy,
                     const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data);
static int draw_cell_dbg(struct tsm_screen *screen, uint32_t id,
                         const uint32_t *ch, size_t len, unsigned int cwidth,
                         unsigned int posx, unsigned int posy,
                         const struct tsm_screen_attr *attr,
                         tsm_age_t age, void *data);


struct Char {
  Char ()
    : ch(' ')
  {}

  void draw (int posx, int posy, double end=-1) {
    if (ch != ' ') {
      std::cout << "<text style=\"font-family: Courier\""
                << " x=\"" << 20+posx*10 << "\""
                << " y=\"" << 20+posy*14 << "\""
                << " visibility=\"hidden\">";

      switch (ch) {
      case '<': std::cout << "&lt;"; break;
      case '>': std::cout << "&gt;"; break;
      default:  std::cout << ch;
      }

      std::cout << std::endl
                << " <set attributeName=\"visibility\" attributeType=\"CSS\" to=\"visible\""
                << " begin=\"start.end+" << begin << "\"";
      if (end>0) {
        std::cout << " dur=\"" << end-begin << "\"";
      }
      std::cout << "/>\n</text>" << std::endl;
    }
  }

  double begin;
  char ch;
};

class Terminal {
public:
  Terminal ()
    : time (-1)
  {
    tsm_screen_new (&screen, log_tsm, NULL);
    tsm_screen_resize(screen, 80, 54);
    tsm_vte_new (&vte, screen,
                 term_write_cb, NULL,
                 log_tsm, NULL);

    ch.resize(80);
    for (uint i=0 ; i<80 ; ++i) {
      ch[i].resize(54);
    }

    std::cout << "<svg xmlns=\"http://www.w3.org/2000/svg\""
              << " xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << std::endl
              << "<text>"
              << "<set id=\"start\" attributeName=\"visibility\" attributeType=\"CSS\" to=\"visible\""
              << " begin=\"0; progress.end+1\" dur=\"0.001\"/>"
              << "</text>" << std::endl;
  }

  ~Terminal () {
    tsm_vte_unref (vte);
    tsm_screen_unref (screen);

    time += 1;
    for (uint posx = 0 ; posx<ch.size() ; posx++) {
      for (uint posy = 0 ; posy<ch[posx].size() ; ++posy) {
        ch[posx][posy].draw(posx, posy, time);
      }
    }
    std::cout << "<rect x=\"20\" y=\"770\" width=\"800\" height=\"5\" style=\"stroke:#0000cc; fill:none\" />" << std::endl
              << "<rect x=\"20\" y=\"770\" height=\"5\" style=\"fill:#0000cc;\">"
              << "<animate id=\"progress\" attributeName=\"width\" attributeType=\"XML\""
              << " from=\"0\"  to=\"800\" fill=\"freeze\""
              << " begin=\"start.end\" dur=\"" << time-1 << "\" />"
              << "</rect>" << std::endl
              << "</svg>";
  }

  void input (const char * input, double delay) {
    if (time == -1)
      time = 0;
    else
      time += delay;
    tsm_vte_input (vte, input, strlen(input));
    tsm_screen_draw (screen, draw_cell, this);
  }

  void draw (uint posx, uint posy, char c) {
    if (posx >= ch.size()) return;
    if (posy >= ch[posx].size()) return;

    Char & cell = ch[posx][posy];
    if (c!=cell.ch) {
      cell.draw(posx, posy, time);
      cell.begin = time;
      cell.ch = c;
    }
  }

  void debug () {
    std::ofstream dbg ("debug.svg", std::ofstream::out);
    dbg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        << " xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";
    tsm_screen_draw (screen, draw_cell_dbg, &dbg);
    dbg << "</svg>";
  }

private:
  struct tsm_vte *vte;
  struct tsm_screen * screen;
  double time;
  std::vector<std::vector<Char>> ch;
};

static int draw_cell(struct tsm_screen *screen, uint32_t id,
                     const uint32_t *ch, size_t len, unsigned int cwidth,
                     unsigned int posx, unsigned int posy,
                     const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data)
{
  Terminal * term = (Terminal*)data;

  if (len) {
    term->draw(posx, posy, static_cast<char>(*ch));
  } else {
    term->draw(posx, posy, ' ');
  }

  return 0;
}


static int draw_cell_dbg(struct tsm_screen *screen, uint32_t id,
                         const uint32_t *ch, size_t len, unsigned int cwidth,
                         unsigned int posx, unsigned int posy,
                         const struct tsm_screen_attr *attr,
                         tsm_age_t age, void *data)
{
  std::ostream & dbg (*static_cast<std::ostream*>(data));

  if (len) {
    dbg << "<text style=\"font-family: Courier\" x=\"" << 20+10*posx
        << "\" y=\"" << 20+12*posy << "\">"
        << static_cast<char>(*ch) << "</text>\n";
  }

  return 0;
}



int main () {
  Terminal term;


  std::ifstream script ("/tmp/example.script", std::ifstream::in);
  char * buffer = new char [4096];
  script.getline(buffer, 4096);

  // buffer[1] = 0;
  // while(true) {
  //   buffer[0] = script.get();
  //   if (script.good()) {
  //     term.input(buffer, 0.01);
  //   } else {
  //     break;
  //   }
  // }

  double delay = 0;
  int nb = 0;
  std::ifstream timing ("/tmp/example.timing", std::ifstream::in);
  while (true) {
    double delay1;
    int nb1;
    timing >> delay1 >> nb1;
    if (timing.good()) {
      delay += delay1;
      nb += nb1;
      if (delay > 0.01) {
        script.read(buffer, nb);
        buffer[nb] = 0;
        //std::cerr << delay << " - [" << buffer << "]" << std::endl;
        term.input (buffer, delay);

        delay = 0;
        nb = 0;
      }
    }
    else
      break;
  }

  term.debug();

  delete[] buffer;

  return 0;
}
