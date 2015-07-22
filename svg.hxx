#pragma once

#include <string>
#include <sstream>

namespace SVG {
class Template
{
public:
  Template (std::string str)
    : str_(str)
  {}

  template <typename T>
  Template & operator() (std::string placeholder, T value)
  {
    std::ostringstream oss;
    oss << value;

    size_t pos = 0;
    while ((pos = str_.find (placeholder, pos)) != std::string::npos) {
      str_.replace (pos, placeholder.size(), oss.str());
    }
    return *this;
  }

  const std::string & str () const
  {
    return str_;
  }

private:
  std::string str_;
};

Template header ()
{
  return Template
    ("<svg xmlns='http://www.w3.org/2000/svg'"
     " xmlns:xlink='http://www.w3.org/1999/xlink'\n"
     " font-family='$FONT' font-size='$SIZE'>\n"
     " <!-- Cycling -->\n"
     " <text>\n"
     "  <set id='start' attributeName='visibility' attributeType='XML' to='visible'"
     "   begin='0; progress.end+1' dur='0'/>\n"
     " </text>\n");
}

Template footer ()
{
  return Template
    ("</svg>\n");
}

Template advertisement ()
{
  return Template
    (" <!-- Advertisement -->\n"
     " <a xlink:href='$URL' xlink:show='new'>\n"
     "  <text x='$X' y='$Y' transform='rotate(-90, $X, $Y)'"
     "   dominant-baseline='text-before-edge' font-size='$SIZE'>\n"
     "   $TEXT\n"
     "  </text>\n"
     " </a>\n");
}

Template progress ()
{
  return Template
    (" <!-- Progress bar -->\n"
     " <!--   border -->\n"
     " <rect x='$X0' y='$Y0' width='$DX' height='$DY'"
     "  style='stroke:#$COLOR; fill:none' />\n"
     " <!--   filling -->\n"
     " <rect x='$X0' y='$Y0' height='$DY' fill='#$COLOR'>\n"
     "  <animate id='progress' attributeName='width' attributeType='XML'"
     "   from='0' to='$DX' fill='freeze'"
     "   begin='start.begin' dur='$TIME' />\n"
     " </rect>\n");
}


Template bgHead ()
{
  return Template
    (" <!-- Background -->\n"
     " <rect x='$X0' y='$Y0' width='$WIDTH' height='$HEIGHT' fill='#$BG'/>\n");
}

Template bgCell ()
{
  return Template
    (" <rect x='$X' y='$Y' width='$DX' height='$DY'"
     "  fill='#$COLOR' visibility='hidden'>\n"
     "  <set attributeType='XML' attributeName='visibility' to='visible'"
     "   begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     " </rect>\n");
}


Template textHead ()
{
  return Template
    (" <!-- Text -->\n"
     " <text dominant-baseline='text-before-edge' fill='#$FG'>\n");
}

Template textFoot ()
{
  return Template
    (" </text>\n");
}


Template textCell ()
{
  return Template
    ("  <tspan x='$X' y='$Y' visibility='hidden'$COLOR$BOLD$UNDER>$CHAR\n"
     "   <set attributeName='visibility' attributeType='XML' to='visible'"
     "    begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     "  </tspan>\n");
}
}
