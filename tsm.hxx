#pragma once

#include "logger.hxx"
#include <libtsm.h>

namespace TSM {

class Screen {
public:
  Screen (Log::Logger & logger);
  ~Screen ();

  // Non-copyable
  Screen (const Screen &) = delete;

  tsm_screen* operator() () const { return screen_; }

private:
  tsm_screen *screen_;
};

class VTE {
public:
  VTE (Log::Logger & logger,
       tsm_screen * screen);

  ~VTE ();

  // Non-copyable
  VTE (const VTE &) = delete;

  tsm_vte* operator() () const { return vte_; }
private:
  tsm_vte *vte_;
};
}
