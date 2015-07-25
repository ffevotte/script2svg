# script2svg

`script2svg` is a simple program allowing to produce SVG animations from
[`script`][script] files. It is intended to be used as an alternative to
animated GIFs when it comes to screencasting terminal-based applications.


## Example

![screencast](http://ffevotte.github.com/script2svg/screencast/screencast.svg)


## Installation

### Dependencies

`script2svg` uses the following tools, which should be installed on your system
before you try building the project:

- a C++14-compliant compiler, such as [gcc][] or [clang][]
- [cmake][]
- [libtsm][]
- [boost][]::[program_options][po]

### Build

Since `script2svg` is based on `cmake`, something along these lines should allow
you to simply build the project:

```shell
$ git clone http://www.github.com/ffevotte/script2svg.git
$ cd script2svg
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Usage

The simplest way of recording a screencast is the one shown in the screencast above:

```shell
$ # Start recording
$ script -t scriptfile 2> timingfile
Script started, file is scriptfile.
$ 
$ # do whatever you want
$ # exit to stop recording
$ exit
exit
Script done, file is scriptfile
$
$ # Produce the SVG animation
$ script2svg scriptfile timingfile -o screencast.svg
info: setting verbosity level to 3(info)
info: setting output to file 'screencast.svg'
info: setting terminal size to 80x24
notice: animation duration: 32.27s. 
```

### Step 1: Record the session

This step is performed using the [`script`][script] utility. See its
[man page][man_script] for more details.

Don't forget to pass the `-t` command-line switch in order to generate a timing
file (and don't forget to redirect the standard error so that timing data are
recorded on disk).

### Step 2: Produce the SVG animation

`script2svg` supports a lot of command-line options allowing to tune the
output. Their usage is described by the in-line help message:


    Usage: script2svg [options] SCRIPT_FILE TIMING_FILE
    
    Produce an animated SVG representation of a recorded script session.
    
    Generic options:
      --help                             produce help message
      -V [ --version ]                   print version
      -v [ --verbose ] [=LEVEL(=4)] (=3) set verbosity level
      -C [ --config ] FILE               read config file
      -o [ --output ] SVG_FILE (=-)      specify the output file name. The default 
                                         behaviour is to use the standard output.
    
    Terminal:
    By default, `script2svg` respectively reads the terminal size from the COLUMNS
    and LINES environment variables. These options allow specifying them explicitly:
      -c [ --columns ] NB (=0)           number of columns
      -r [ --rows ]    NB (=0)           number of rows
    
    Fonts:
      --font.family FONT (=monospace)    font family
      --font.size   SIZE (=12)           font size
      --font.dx     PX (=0)              horizontal cell size
      --font.dy     PX (=0)              vertical cell size.
                                         The default behaviour is to try and 
                                         automatically determine the cell size based 
                                         on the font size.
    
    Progress bar:
      --progress.height PX (=5)          progress bar height
      --progress.color  HEX (=0000aa)    progress bar color
    
    Colors:
      --color.fg      HEX_CODE (=000000) foreground color
      --color.bg      HEX_CODE (=ffffff) background color
      --color.black   HEX_CODE (=000000) black / dark gray
      --color.red     HEX_CODE (=aa0000) red
      --color.green   HEX_CODE (=00aa00) green
      --color.yellow  HEX_CODE (=aa5500) green
      --color.blue    HEX_CODE (=0000aa) blue
      --color.magenta HEX_CODE (=aa00aa) magenta
      --color.cyan    HEX_CODE (=00aaaa) cyan
      --color.white   HEX_CODE (=aaaaaa) white / light gray
    
    Advertisement:
    A link is inserted at the bottom right corner of the generated SVG animation.
    The following options allow customizing it:
      --ad.text TEXT                     advertisement text; if TEXT is blank, 
                                         no advertisement is produced.
      --ad.url URL                       advertisement URL



## Bugs / TODO-list

- non-ASCII characters are not yet supported


## Contributing

If you make improvements to this code or have suggestions, please do not
hesitate to fork the repository or submit bug reports on
[github](https://github.com/ffevotte/script2svg). The repository's URL is:

    https://github.com/ffevotte/script2svg.git


## License

Copyright (C) 2015 François Févotte.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.


[script]: http://en.wikipedia.org/wiki/Script_(Unix)
[libtsm]: http://www.freedesktop.org/wiki/Software/kmscon/libtsm/
[boost]:  http://www.boost.org/
[po]:     http://www.boost.org/doc/libs/1_58_0/doc/html/program_options.html
[cmake]:  http://www.cmake.org/
[gcc]:    http://gcc.gnu.org/
[clang]:  http://clang.llvm.org/
[man_script]: http://www.unix.com/man-page/all/1/script/
