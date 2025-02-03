#include <iostream>
#include <filesystem>
#include <thread>

#include "conf/options_parser.h"

namespace fs = std::filesystem;

class settings : public conf::configuration {
public:
  explicit settings() : configuration("Options") {
    param(config_file, "config,c", "Config file path");
  }

  fs::path config_file{"config.yaml"};
};

int main(int argc, char const* argv[]) {
  std::cout << "Hello, World!" << std::endl;
  auto opt = settings{};
  auto parser = conf::options_parser({&opt});
  parser.read_command_line_args(argc, argv);

  if (parser.help()) {
    parser.print_help(std::cout);
    return 0;
  } else if (parser.version()) {
    std::cout << parser.version() << std::endl;
    return 0;
  }
  std::cout << "Config file: " << opt.config_file << std::endl;
  return 0;
}
