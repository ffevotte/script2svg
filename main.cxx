#include "logger.hxx"
#include "terminal.hxx"
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>

int main (int argc, char **argv) {
  namespace po = boost::program_options;

  using Log::ERROR;
  using Log::WARNING;
  using Log::INFO;
  Log::Logger log {std::cerr};
  //log.level (Log::LEVEL_MAX);

  po::options_description optionsAll;
  po::options_description optionsDoc;

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

  po::options_description optionsGeneric {"Generic options"};
  optionsGeneric.add_options()
    ("help",    "produce help message")
    ("version,V", "print version")
    ("verbose,v",
     po::value<int>()
     ->value_name("LEVEL")
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

  po::options_description optionsTerm {"Terminal options"};
  optionsTerm.add_options()
    ("columns,c",
     po::value<int>()
     ->value_name("NB")
     ->default_value(80),
     "number of columns")
    ("rows,r",
     po::value<int>()
     ->value_name("   NB")
     ->default_value(24),
     "number of rows");
  optionsAll.add (optionsTerm);
  optionsDoc.add (optionsTerm);

  po::options_description optionsStyle {"Styling options"};
  optionsStyle.add_options()
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
  optionsAll.add (optionsStyle);
  optionsDoc.add (optionsStyle);

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

  // Handle simple options early
  if (vm.count("help")) {
    help (std::cout);
    return 0;
  }

  if (vm.count("verbose")) {
    log.level (Log::Level(vm["verbose"].as<int>()));
  }

  try {
    po::notify(vm);
  } catch (po::error & e) {
    log.write<ERROR> ([&](auto&&out) {
        out << e.what() << std::endl;
        help (out);
      });
    return 1;
  }

  if (vm.count("config")) {
    std::string fileName {vm["config"].as<std::string>()};
    log.write<INFO> ([&](auto && out){
        out << "reading configuration file: '"
            << fileName
            << "'" << std::endl;
      });
    try {
      po::store
        (po::parse_config_file<char> (fileName.c_str(),
                                      optionsDoc),
         vm);
    } catch (po::reading_file & e){
      log.msg<WARNING> (e.what());
    } catch (po::error & e) {
      log.msg<ERROR> (e.what());
      return 1;
    }
    po::notify (vm);
  }

  std::ofstream svg ("essai2.svg");
  Terminal term (log, svg);
  term.play("/tmp/example.script", "/tmp/example.timing");


  std::cout << "style.blue = " << vm["style.blue"].as<std::string>() << std::endl;
  std::cout << "script = " << vm["script-file"].as<std::string>() << std::endl;
  std::cout << "timing = " << vm["timing-file"].as<std::string>() << std::endl;

  return 0;
}
