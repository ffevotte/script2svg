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

template<Level L> constexpr const char * staLevelName  () { return "default"; }
template<> constexpr const char* staLevelName<ERROR>   () { return "error"; }
template<> constexpr const char* staLevelName<WARNING> () { return "warning"; }
template<> constexpr const char* staLevelName<NOTICE>  () { return "notice"; }
template<> constexpr const char* staLevelName<INFO>    () { return "info"; }
template<> constexpr const char* staLevelName<DEBUG>   () { return "debug"; }

template<Level CUR>
inline const char * dynLevelName (Level lvl) {
  if (lvl == CUR) {
    return staLevelName<CUR>();
  }
  return dynLevelName<Level(CUR+1)> (lvl);
}

template<>
inline const char * dynLevelName<LEVEL_MAX> (Level lvl) {
  return staLevelName<LEVEL_MAX>();
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

  Level level () const {
    return level_;
  }

  const char * levelName () const {
    return dynLevelName<Level(0)>(level_);
  }

  template <typename T>
  void msg (Level lvl, const T& t) {
    write (lvl, [&](auto&&out){
        out < t << std::endl;
      });
  }

  void write (Level lvl, std::function<void(std::ostream & out)> f) {
    if (lvl <= level_) {
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
      f ((*out_) << staLevelName<LVL>() << ": ");
    }
  }

private:
  std::ostream * out_;
  Level level_;
};
}
