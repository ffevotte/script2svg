#pragma once

#include "memory.hxx"
#include "logger.hxx"
#include "tsm.hxx"
#include <vector>
#include <boost/program_options.hpp>

class Terminal;

struct Cell {
  struct Prop {
    bool operator!= (const Prop & other) const;
    int  fg;
    int  bold;
    int  underline;
  };

  char ch;
  Prop prop;
  int  bg;
};

class AnimatedRow {
public:
  void init (Terminal * term, uint row) {
    term_ = term;
    row_  = row;
  }

  void update ();
  void draw () const;

protected:
  virtual std::string state () const = 0;
  virtual void drawState (const std::string & state,
                          double begin, double dur) const = 0;
  const Terminal * term_;
  uint row_;

private:
  struct TimedState {
    std::string state;
    double begin;
    double end;
  };

  std::vector<TimedState> tstate_;
};

class RowText : public AnimatedRow {
private:
  std::string state () const;
  void drawState (const std::string & state,
                  double begin, double dur) const;
};

class RowBg : public AnimatedRow {
private:
  std::string state () const;
  void drawState (const std::string & state,
                  double begin, double dur) const;
};

class Terminal {
public:
  struct Options;

  Terminal (Options & opt,
            Log::Logger & log);

  ~Terminal ();

  void play (const std::string & scriptPath, const std::string timingPath);

  std::ostream & out () const {return *(out_.get());}
  double time () const {return time_;}

  const std::vector<Cell> & cellRow (int row) const {
    return cell_[row];
  }

  const Options & opt () const {return opt_;}

private:
  void update ();

  // Static wrapper for C-style callbacks
  static int update (struct tsm_screen *screen, uint32_t id,
                     const uint32_t *ch, size_t len, unsigned int cwidth,
                     unsigned int posx, unsigned int posy,
                     const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data);

  Options &            opt_;
  Log::Logger &        log_;
  POptr<std::ostream>  out_;
  TSM::Screen          screen_;
  TSM::VTE             vte_;
  double               time_;
  double               lastUpdate_;
  std::vector<RowText> rowText_;
  std::vector<RowBg>   rowBg_;
  std::vector<std::vector<Cell>> cell_;
};

struct Terminal::Options {
  std::string output;

  // Terminal
  int columns;
  int rows;

  // Fonts
  struct Font {
    std::string family;
    int         size;
    int         dx;
    int         dy;
  };
  Font font;

  // Progress bar
  struct Progress {
    int         height;
    std::string color;
  };
  Progress progress;

  // Colors
  struct Color {
    std::string fg;
    std::string bg;
    std::string black;
    std::string red;
    std::string green;
    std::string yellow;
    std::string blue;
    std::string magenta;
    std::string cyan;
    std::string white;
  };
  Color color;

  // Advertisement
  struct Ad {
    std::string text;
    std::string url;
  };
  Ad ad;
};
