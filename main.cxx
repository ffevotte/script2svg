#include "string.hxx"
#include "logger.hxx"
#include "terminal.hxx"
#include "config.h"
#include <iostream>
#include <boost/program_options.hpp>

int main (int argc, char **argv)
{
  using Log::ERROR;
  using Log::WARNING;
  using Log::NOTICE;
  using Log::INFO;
  Log::Logger log {std::cerr};

  try {
// * Options definition
    using std::string;
    namespace po = boost::program_options;
    auto & m_default_line_length {po::options_description::m_default_line_length};
    po::options_description optionsAll;
    po::options_description optionsDoc;

// ** Positional arguments
    po::options_description optionsPositional;
    optionsPositional.add_options()
      ("script-file",
       po::value<string>()
       ->required(),
       "Script file")
      ("timing-file",
       po::value<string>()
       ->required(),
       "Timing file");
    optionsAll.add (optionsPositional);
    po::positional_options_description positional;
    positional
      .add ("script-file", 1)
      .add ("timing-file", 1);

// ** Generic Options
    po::options_description optionsGeneric {"Generic options"};
    optionsGeneric.add_options()
      ("help",    "produce help message")
      ("version,V", "print version")
      ("verbose,v",
       po::value<int>()
       ->value_name("LEVEL")
       ->default_value (Log::INFO)
       ->implicit_value (Log::DEBUG),
       "set verbosity level")
      ("config,C",
       po::value<string>()
       ->value_name("FILE"),
       "read config file")
      ("output,o",
       po::value<string>()
       ->value_name("SVG_FILE"),
       "specify the output file name. The default behaviour is to use"
       " the standard output.");
    optionsAll.add (optionsGeneric);
    optionsDoc.add (optionsGeneric);

// ** Terminal
    po::options_description optionsTerm {
      String ("Terminal:\n"
              "By default, `script2svg` respectively reads the terminal size from the COLUMNS"
              " and LINES environment variables. These options allow specifying them explicitly")
        .wordWrap (m_default_line_length)
        .str()};
    optionsTerm.add_options()
      ("columns,c",
       po::value<int>()
       ->value_name("NB")
       ->default_value(0),
       "number of columns")
      ("rows,r",
       po::value<int>()
       ->value_name("   NB")
       ->default_value(0),
       "number of rows");
    optionsAll.add (optionsTerm);
    optionsDoc.add (optionsTerm);

// ** Fonts
    po::options_description optionsFont {"Fonts"};
    optionsFont.add_options()
      ("font",
       po::value<string>()
       ->value_name("FONT")
       ->default_value("monospace"),
       "font family")
      ("size",
       po::value<int>()
       ->value_name("SIZE")
       ->default_value(12),
       "font size")
      ("dx",
       po::value<int>()
       ->value_name("PX")
       ->default_value(8),
       "horizontal cell size")
      ("dy",
       po::value<int>()
       ->value_name("PX")
       ->default_value(14),
       "vertical cell size");
    optionsAll.add (optionsFont);
    optionsDoc.add (optionsFont);

// ** Progress bar
    po::options_description optionsProgress {"Progress bar"};
    optionsProgress.add_options()
      ("progress.height",
       po::value<int>()
       ->value_name ("PX")
       ->default_value (5),
       "progress bar height")
      ("progress.color",
       po::value<string>()
       ->value_name (" HEX")
       ->default_value ("0000aa"),
       "progress bar color");
    optionsAll.add (optionsProgress);
    optionsDoc.add (optionsProgress);

// ** Colors
    po::options_description optionsColors {"Colors"};
    optionsColors.add_options()
      ("color.fg",
       po::value<string>()
       ->default_value ("000000")
       ->value_name ("     HEX_CODE"),
       "foreground color")
      ("color.bg",
       po::value<string>()
       ->default_value ("ffffff")
       ->value_name ("     HEX_CODE"),
       "background color")
      ("color.black",
       po::value<string>()
       ->default_value ("000000")
       ->value_name ("  HEX_CODE"),
       "black / dark gray")
      ("color.red",
       po::value<string>()
       ->default_value ("aa0000")
       ->value_name ("    HEX_CODE"),
       "red")
      ("color.green",
       po::value<string>()
       ->default_value ("00aa00")
       ->value_name ("  HEX_CODE"),
       "green")
      ("color.yellow",
       po::value<string>()
       ->default_value ("aa5500")
       ->value_name (" HEX_CODE"),
       "green")
      ("color.blue",
       po::value<string>()
       ->default_value ("0000aa")
       ->value_name ("   HEX_CODE"),
       "blue")
      ("color.magenta",
       po::value<string>()
       ->default_value ("aa00aa")
       ->value_name ("HEX_CODE"),
       "magenta")
      ("color.cyan",
       po::value<string>()
       ->default_value ("00aaaa")
       ->value_name ("   HEX_CODE"),
       "cyan")
      ("color.white",
       po::value<string>()
       ->default_value ("ffffff")
       ->value_name ("  HEX_CODE"),
       "white / light gray");
    optionsAll.add (optionsColors);
    optionsDoc.add (optionsColors);

// ** Advertisement
    po::options_description optionsAd {
        String("Advertisement:\n"
               "A link is inserted at the bottom right corner of the generated SVG"
               " animation. The following options allow customizing it")
          .wordWrap (m_default_line_length)
          .str()};
    optionsAd.add_options()
      ("ad.text",
       po::value<string>()
       ->value_name ("TEXT")
       ->default_value ("Produced by script2svg"),
       "advertisement text; if TEXT is blank, no advertisement is produced.")
      ("ad.url",
       po::value<string>()
       ->value_name ("URL")
       ->default_value ("http://github.com/ffevotte/script2svg"),
       "advertisement URL");
    optionsAll.add (optionsAd);
    optionsDoc.add (optionsAd);


// * Command-line parsing (step 1)
    auto help = [&](std::ostream & out) {
      out
      << "Usage: " << argv[0] << " [options] SCRIPT_FILE TIMING_FILE" << std::endl
      << std::endl
      << String ("Produce an animated SVG representation of a recorded script session.")
      .wordWrap (m_default_line_length)
      .str() << std::endl
      << optionsDoc << std::endl;
    };

    po::variables_map vm;
    try {
      po::store
        (po::command_line_parser (argc, argv)
         .options (optionsAll)
         .positional (positional)
         .run(),
         vm);
    } catch (po::error & e) {
      log.write<WARNING> ([&](auto&&out) {
          out << e.what() << std::endl;
          help (out);
        });
    }

// * Early processing of simple, generic options

    // --help
    if (vm.count ("help")) {
      help (std::cout);
      return 0;
    }

    // --version
    if (vm.count ("version")) {
      std::cout << "script2svg "
                << SCRIPT2SVG_VERSION_MAJOR << "." << SCRIPT2SVG_VERSION_MINOR
                << std::endl;
      return 0;
    }

    // --verbose
    {
      log.level (Log::Level(vm["verbose"].as<int>()));
      log.write<INFO> ([&](auto&&out){
          out << "setting verbosity level to " << log.level()
              << "(" << log.levelName() << ")" << std::endl;
        });
    }

// * Command-line parsing (step 2)
    try {
      po::notify(vm);
    } catch (po::error & e) {
      log.write<ERROR> ([&](auto&&out) {
          out << e.what() << std::endl;
          help (out);
        });
      return 2;
    }

// * Configuration file
    if (vm.count("config")) {
      string fileName {vm["config"].as<string>()};
      log.write<NOTICE> ([&](auto && out){
          out << "reading configuration file: '" << fileName << "'"
              << std::endl;
        });
      try {
        po::store (po::parse_config_file<char> (fileName.c_str(), optionsDoc),
                   vm);
      } catch (po::reading_file & e){
        log.msg<WARNING> (e.what());
      } catch (po::error & e) {
        log.msg<ERROR> (e.what());
        return 3;
      }
      po::notify (vm);
    }

// * Real work
    Terminal term (vm, log);
    term.play(vm["script-file"].as<string>(),
              vm["timing-file"].as<string>());
  } catch (std::runtime_error & e) {
    log.msg<ERROR> (e.what());
    return 1;
  }

  return 0;
}
