#pragma once

#include "logger.hxx"
#include "tsm.hxx"
#include <vector>
#include <boost/program_options.hpp>

class Terminal;

class Cell {
public:
  struct State {
    State ();
    bool operator!= (const State & other) const;
    char ch;
  };

  void term (const Terminal * term);
  void update (const State & state);
  void draw (uint col, uint row) const;

private:
  struct TimedState {
    State state;
    double begin;
    double end;
  };

  const Terminal * term_;
  std::vector<TimedState> tstate_;
};


class Cursor {
public:
  struct Position {
    bool operator!= (const Position & other) const {
      return (x != other.x or y!=other.y);
    }

    unsigned int x;
    unsigned int y;
  };

  void term (const Terminal * term) {term_ = term;}
  void update (const Position & pos);
  void draw () const;

private:
  struct TimedPosition {
    Position pos;
    double   begin;
    double   end;
  };

  const Terminal * term_;
  std::vector<TimedPosition> tpos_;
};

// Possibly Owning ptr
template <typename T>
class POptr {
public:
  POptr ()
    : ptr_ (0),
      owner_ (false)
  {}

  POptr (T* ptr, bool owner)
    : ptr_ (ptr),
      owner_ (owner)
  {}

  // Non copyable
  POptr (const POptr & other) = delete;

  ~POptr () {
    if (owner_)
      delete (ptr_);
  }

  void reset (T* ptr, bool owner) {
    if (owner_)
      delete (ptr_);
    ptr_   = ptr;
    owner_ = owner;
  }

  T* get () const { return ptr_; }

private:
  T * ptr_;
  bool owner_;
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

private:
  void update ();
  void update (uint col, uint row, const Cell::State & state);

  // Static wrapper for C-style callbacks
  static int update (struct tsm_screen *screen, uint32_t id,
                     const uint32_t *ch, size_t len, unsigned int cwidth,
                     unsigned int posx, unsigned int posy,
                     const struct tsm_screen_attr *attr,
                     tsm_age_t age, void *data);

  const boost::program_options::variables_map & vm_;
  Log::Logger &       log_;
  POptr<std::ostream> out_;
  TSM::Screen         screen_;
  TSM::VTE            vte_;
  double              time_;
  double              lastUpdate_;
  Cursor              cursor_;
public:
  std::vector<std::vector<Cell>> cell_;
};
