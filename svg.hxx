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
     " font-family='$FONT' font-size='$SIZE' fill='#$FG'"
     " width='$WIDTH' height='$HEIGHT'>\n"
     "<!-- Cycling -->\n"
     "<text>\n"
     " <set id='start' attributeName='visibility' attributeType='XML' to='visible'"
     "  begin='0; progress.end+0.01' dur='0'/>\n"
     "</text>\n");
}

Template footer ()
{
  return Template
    ("</svg>\n");
}

Template advertisement ()
{
  return Template
    ("<!-- Advertisement -->\n"
     "<a xlink:href='$URL' xlink:show='new'>\n"
     " <text x='$X' y='$Y' transform='rotate(-90, $X, $Y)' fill='#000000'"
     "  dominant-baseline='text-before-edge' font-size='$SIZE'>\n"
     "  $TEXT\n"
     " </text>\n"
     "</a>\n");
}

Template progress ()
{
  return Template
    ("<!-- Progress bar -->\n"
     "<rect x='$X0' y='$Y0' width='$DX' height='$DY'"
     " style='stroke:#$COLOR; fill:none' />\n"
     "<rect x='$X0' y='$Y0' height='$DY' fill='#$COLOR'>\n"
     " <animate id='progress' attributeName='width' attributeType='XML'"
     "  from='0' to='$DX' fill='freeze'"
     "  begin='start.begin' dur='$TIME' />\n"
     "</rect>\n");
}


Template bgHead ()
{
  return Template
    ("<!-- Background -->\n"
     "<rect x='0' y='0' width='$WIDTH' height='$HEIGHT' fill='#$BG'/>\n");
}

Template rowBg ()
{
  return Template
    ("<g display='none'>\n"
     " $BG\n"
     " <set attributeType='XML' attributeName='display' to='inline'"
     "  begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     "</g>\n");
}

Template bg ()
{
  return Template
    ("<rect x='$X' y='$Y' width='$WIDTH' height='$DY' fill='#$COLOR'/>");
}


Template textHead ()
{
  return Template
    (" <!-- Text -->\n");
}

Template rowText ()
{
  return Template
    ("<text x='$X' y='$Y' dominant-baseline='text-before-edge' textLength='$WIDTH'"
     " display='none'>\n"
     " $TEXT\n"
     " <set attributeType='XML' attributeName='display' to='inline'"
     "  begin='start.begin+$BEGIN' dur='$DUR'/>\n"
     "</text>\n");
}

Template propHead () {
  return Template
    ("<tspan$COLOR$BOLD$UNDERLINE>");
}

Template propFoot () {
  return Template
    ("</tspan>");
}
}
