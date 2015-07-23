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
  Terminal (const boost::program_options::variables_map & vm,
            Log::Logger & log);

  ~Terminal ();

  void play (const std::string & scriptPath, const std::string timingPath);

  std::ostream & out () const {return *(out_.get());}
  double time () const {return time_;}

  template <typename T>
  const T & vm (const std::string & key) const {
    return vm_[key].as<T>();
  }

  const std::vector<Cell> & cellRow (int row) const {
    return cell_[row];
  }

  unsigned int nRows () const {
    return cell_.size();
  }

  unsigned int nCols () const {
    return cell_[0].size();
  }

private:
  void update ();

  // Static wrapper for C-style callbacks
  static int update (struct tsm_screen *screen, uint32_t id,
                     const uint32_t *ch, size_t len, unsigned int cwidth,
                     unsigned int posx, unsigned int posy,
                     const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data);

  const boost::program_options::variables_map & vm_;
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
