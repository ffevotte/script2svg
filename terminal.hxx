#pragma once

#include "logger.hxx"
#include "tsm.hxx"
#include <vector>


class Cell {
public:
  struct State {
    State ();
    bool operator!= (const State & other) const;
    char ch;
  };

  void pos (unsigned int x, unsigned int y);

  void update (std::ostream & out, double time, const State & state);

  void draw (std::ostream & out, double end);

private:
  unsigned int x_;
  unsigned int y_;
  double       begin_;
  State        state_;
};


class Terminal {
public:
  Terminal (Log::Logger & log, std::ostream & out);

  ~Terminal ();

  void play (const std::string & scriptPath, const std::string timingPath);

private:
  void update ();
  void update (uint col, uint row, const Cell::State & state);

  // Static wrapper for C-style callbacks
  static int update (struct tsm_screen *screen, uint32_t id,
                     const uint32_t *ch, size_t len, unsigned int cwidth,
                     unsigned int posx, unsigned int posy,
                     const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data);

  Log::Logger &  log_;
  std::ostream & out_;
  TSM::Screen    screen_;
  TSM::VTE       vte_;
  double         time_;
  std::vector<std::vector<Cell>> cell_;
};
