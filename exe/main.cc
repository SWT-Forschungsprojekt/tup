#include <iostream>
#include <filesystem>

#include "tup/import.h"
#include "tup/data.h"
#include "conf/options_parser.h"
#include "utl/progress_tracker.h"

namespace po = boost::program_options;
namespace fs = std::filesystem;

using namespace tup;

class settings final : public conf::configuration {
public:
  explicit settings() : configuration("Options") {
    param(config_file, "config,c", "Config file path");
  }

  fs::path config_file{"config.yaml"};
};

int main(const int argc, char const* argv[]) {
  auto opt = settings{};
  auto parser = conf::options_parser({&opt});
  parser.read_command_line_args(argc, argv);

  if (parser.help()) {
    parser.print_help(std::cout);
    return 0;
  }
  if (parser.version()) {
    std::cout << parser.version() << std::endl;
    return 0;
  }
  std::cout << "Config file: " << opt.config_file << std::endl;
  auto c = config{};
  try {
    auto data_path = fs::path{"data"};
    auto config_path = fs::path{"config.yml"};
    auto desc = po::options_description{"Import Options"};
    auto const bars = utl::global_progress_bars{false};
    import(c, std::move(data_path));
    std::cout << "Successfully imported GTFS data";
    return 0;
  } catch (std::exception const&) {
    std::cout << "An error occurred";
    return 1;
  }
  return 0;
}
