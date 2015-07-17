#pragma once

#include <iostream>
#include <functional>

namespace Log {

enum Level {
  ERROR,
  WARNING,
  NOTICE,
  INFO,
  DEBUG,
  LEVEL_MAX
};

template<Level L> constexpr const char * levelName  () { return "default"; }
template<> constexpr const char* levelName<ERROR>   () { return "error"; }
template<> constexpr const char* levelName<WARNING> () { return "warning"; }
template<> constexpr const char* levelName<NOTICE>  () { return "notice"; }
template<> constexpr const char* levelName<INFO>    () { return "info"; }
template<> constexpr const char* levelName<DEBUG>   () { return "debug"; }

template<Level CUR>
inline const char * dynLevelName (Level lvl) {
  if (lvl == CUR) {
    return levelName<CUR>();
  }
  return dynLevelName<Level(CUR+1)> (lvl);
}

template<>
inline const char * dynLevelName<LEVEL_MAX> (Level lvl) {
  return levelName<LEVEL_MAX>();
}

class Logger {
public:
  Logger (std::ostream & out = std::cout)
    : out_   (&out),
      level_ (INFO)
  {}

  void to (std::ostream & out) {
    out_ = &out;
  }

  void level (Level l) {
    level_ = l;
  }

  template <typename T>
  void msg (Level lvl, const T& t) {
    write (lvl, [&](auto&&out){
        out < t << std::endl;
      });
  }

  void write (Level lvl, std::function<void(std::ostream & out)> f) {
    if (lvl <= 100*level_) {
      f ((*out_) << dynLevelName<Level(0)>(lvl) << ": ");
    }
  }

  template <Level LVL, typename T>
  void msg (const T& t) {
    write<LVL> ([&](auto&&out){
        out << t << std::endl;
      });
  }

  template <Level LVL>
  void write (std::function<void(std::ostream & out)> f) {
    if (LVL <= level_) {
      f ((*out_) << levelName<LVL>() << ": ");
    }
  }

private:
  std::ostream * out_;
  Level level_;
};
}
