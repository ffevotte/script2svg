#include "logger.hxx"
#include "terminal.hxx"
#include <iostream>
#include <fstream>


int main () {
  Log::Logger log (std::cerr);
  log.level (Log::LEVEL_MAX);

  std::ofstream svg ("essai2.svg");

  Terminal term (log, svg);
  term.play("/tmp/example.script", "/tmp/example.timing");

  return 0;
}
