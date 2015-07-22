#pragma once

#include <string>

class String {
public:
  String (const std::string & str)
    : str_ (str)
  {}

  String & wordWrap (int width) {
    using size_type = std::string::size_type;

    size_type lastBreak = 0;
    while (true) {
      size_type eol = str_.find ('\n', lastBreak);
      if (eol == std::string::npos)
        eol = str_.size();

      bool breakInserted = false;
      if (eol - lastBreak > width) {
        size_type space = str_.rfind (' ', lastBreak+width);
        if (space == std::string::npos
            or space <= lastBreak) {
          space = str_.find (' ', lastBreak+width);
        }
        if (space != std::string::npos) {
          lastBreak = space;
          str_[space] = '\n';
          breakInserted = true;
        }
      }

      if (not breakInserted) {
        if (eol == str_.size()) {
          return *this;
        } else {
          lastBreak = eol + 1;
        }
      }
    }
  }

  const std::string & str() const {
    return str_;
  }

private:
  std::string str_;
};
