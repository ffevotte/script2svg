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
    namespace po = boost::program_options;
    po::options_description optionsAll;
    po::options_description optionsDoc;

// ** Positional arguments
    po::options_description optionsPositional;
    optionsPositional.add_options()
      ("script-file",
       po::value<std::string>()
       ->required(),
       "Script file")
      ("timing-file",
       po::value<std::string>()
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
       po::value<std::string>()
       ->value_name("FILE"),
       "read config file")
      ("output,o",
       po::value<std::string>()
       ->value_name("SVG_FILE"),
       "specify the output file name. The default behaviour is to use"
       " the standard output.");
    optionsAll.add (optionsGeneric);
    optionsDoc.add (optionsGeneric);

// ** Terminal
    po::options_description optionsTerm {"Terminal"};
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
       "number of rows.\n"
       "With the default value (0), `script2svg` respectively reads the numbers "
       "of columns and rows from the COLUMNS and LINES environment variables.");
    optionsAll.add (optionsTerm);
    optionsDoc.add (optionsTerm);

// ** Fonts
    po::options_description optionsFont {"Fonts"};
    optionsFont.add_options()
      ("font",
       po::value<std::string>()
       ->value_name("FONT")
       ->default_value("monospace"),
       "font family")
      ("size",
       po::value<std::string>()
       ->value_name("SIZE")
       ->default_value("12"),
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
       po::value<std::string>()
       ->value_name ("PX")
       ->default_value ("5"),
       "progress bar height")
      ("progress.color",
       po::value<std::string>()
       ->value_name (" HEX")
       ->default_value ("0000aa"),
       "progress bar color");
    optionsAll.add (optionsProgress);
    optionsDoc.add (optionsProgress);

// ** Colors
    po::options_description optionsColors {"Colors"};
    optionsColors.add_options()
      ("style.red",
       po::value<std::string>()
       ->default_value ("aa0000")
       ->value_name ("    HEX_CODE"),
       "red color")
      ("style.green",
       po::value<std::string>()
       ->default_value ("00aa00")
       ->value_name ("  HEX_CODE"),
       "green color")
      ("style.yellow",
       po::value<std::string>()
       ->default_value ("aa5500")
       ->value_name (" HEX_CODE"),
       "green color")
      ("style.blue",
       po::value<std::string>()
       ->default_value ("0000aa")
       ->value_name ("   HEX_CODE"),
       "blue color")
      ("style.magenta",
       po::value<std::string>()
       ->default_value ("aa00aa")
       ->value_name ("HEX_CODE"),
       "magenta color")
      ("style.cyan",
       po::value<std::string>()
       ->default_value ("00aaaa")
       ->value_name ("   HEX_CODE"),
       "cyan color");
    optionsAll.add (optionsColors);
    optionsDoc.add (optionsColors);

// * Command-line parsing (step 1)
    auto help = [&](std::ostream & out) {
      out
      << "Usage: " << argv[0] << " [options] SCRIPT_FILE TIMING_FILE" << std::endl
      << std::endl
      << "Produce an animated SVG representation of a recorded script session." << std::endl
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
      return 1;
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
      return 1;
    }

// * Configuration file
    if (vm.count("config")) {
      std::string fileName {vm["config"].as<std::string>()};
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
        return 1;
      }
      po::notify (vm);
    }

// * Real work
    Terminal term (vm, log);
    term.play(vm["script-file"].as<std::string>(),
              vm["timing-file"].as<std::string>());
  } catch (std::runtime_error & e) {
    log.msg<ERROR> (e.what());
    return 1;
  }

  return 0;
}
