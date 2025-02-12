#include <iostream>
#include <filesystem>
#include <thread>

#include "fmt/std.h"

#include "net/stop_handler.h"

#include "conf/options_parser.h"

#include "tup/backend/http_server.h"

#include <vector>

#include "boost/algorithm/string.hpp"
#include "boost/program_options.hpp"

#include "date/date.h"

#include "utl/progress_tracker.h"

#include "nigiri/loader/load.h"
#include "nigiri/loader/loader_interface.h"
#include "nigiri/common/parse_date.h"
#include "nigiri/shapes_storage.h"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using namespace nigiri;
using namespace nigiri::loader;
using namespace std::string_literals;

using namespace tup::backend;

class settings : public conf::configuration {
public:
  explicit settings() : configuration("Options") {
    param(config_file, "config,c", "Config file path");

    param(data_dir_, "data,d", "Data directory");
    param(http_host_, "host,h", "HTTP host");
    param(http_port_, "port,p", "HTTP port");
    param(static_file_path_, "static,s", "Path to static files (ui/web)");
    param(threads_, "threads,t", "Number of routing threads");
    param(lock_, "lock,l", "Lock to memory");
  }

  fs::path config_file{"config.yaml"};

  fs::path data_dir_{"tup"};
  std::string http_host_{"0.0.0.0"};
  std::string http_port_{"8000"};
  std::string static_file_path_;
  bool lock_{true};
  unsigned threads_{std::thread::hardware_concurrency()};
};

auto run(boost::asio::io_context& ioc) {
  return [&ioc]() {
    while (true) {
      try {
        ioc.run();
        break;
      } catch (std::exception const& e) {
        fmt::println("unhandled error: {}", e.what());
      } catch (...) {
        fmt::println("unhandled unknown error");
      }
    }
  };
}
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
  auto const progress_tracker = utl::activate_progress_tracker("importer");
  auto const silencer = utl::global_progress_bars{true};

  auto in = fs::path{};
  auto out = fs::path{"tt.bin"};
  auto out_shapes = fs::path{"shapes"};
  auto start_date = "TODAY"s;
  auto assistance_path = fs::path{};
  auto n_days = 365U;
  auto recursive = false;
  auto ignore = false;

  auto finalize_opt = finalize_options{};
  auto c = loader_config{};

  auto desc = bpo::options_description{"Options"};
  desc.add_options()  //
      ("help,h", "produce this help message")  //
      ("in,i", bpo::value(&in), "input path")  //
      ("recursive,r", bpo::bool_switch(&recursive)->default_value(false),
       "read all zips and directories from the input directory")  //
      ("ignore", bpo::bool_switch(&ignore)->default_value(false),
       "ignore if a directory entry is not a timetable (only for recursive)")  //
      ("out,o", bpo::value(&out)->default_value(out), "output file path")  //
      ("start_date,s", bpo::value(&start_date)->default_value(start_date),
       "start date of the timetable, format: YYYY-MM-DD")  //
      ("num_days,n", bpo::value(&n_days)->default_value(n_days),
       "the length of the timetable in days")  //
      ("tz,t", bpo::value(&c.default_tz_)->default_value(c.default_tz_),
       "the default timezone")  //
      ("link_stop_distance",
       bpo::value(&c.link_stop_distance_)->default_value(c.link_stop_distance_),
       "the maximum distance at which stops in proximity will be linked")  //
      ("merge_dupes_intra_source",
       bpo::value(&finalize_opt.merge_dupes_intra_src_)
           ->default_value(finalize_opt.merge_dupes_intra_src_),
       "merge duplicates within a source")  //
      ("merge_dupes_inter_source",
       bpo::value(&finalize_opt.merge_dupes_inter_src_)
           ->default_value(finalize_opt.merge_dupes_inter_src_),
       "merge duplicates between different sources")  //
      ("adjust_footpaths",
       bpo::value(&finalize_opt.adjust_footpaths_)
           ->default_value(finalize_opt.adjust_footpaths_),
       "adjust footpath lengths")  //
      ("max_foopath_length",
       bpo::value(&finalize_opt.max_footpath_length_)
           ->default_value(finalize_opt.max_footpath_length_))  //
      ("assistance_times", bpo::value(&assistance_path))  //
      ("shapes", bpo::value(&out_shapes));
  auto const pos = bpo::positional_options_description{}.add("in", -1);

  auto vm = bpo::variables_map{};
  bpo::store(
      bpo::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);
  bpo::notify(vm);

  if (vm.count("help") != 0U) {
    std::cout << desc << "\n";
    return 0;
  }

  auto input_files = std::vector<std::pair<std::string, loader_config>>{};
  if (is_directory(in) && recursive) {
    for (auto const& e : fs::directory_iterator(in)) {
      if (is_directory(e) /* unpacked zip file */ ||
          boost::algorithm::to_lower_copy(
              e.path().extension().generic_string()) == ".zip") {
        input_files.emplace_back(e.path().generic_string(), c);
      }
    }
  } else if (exists(in) && !recursive) {
    input_files.emplace_back(in.generic_string(), c);
  }

  if (input_files.empty()) {
    std::cerr << "no input path found\n";
    return 1;
  }

  auto assistance = std::unique_ptr<assistance_times>{};
  if (vm.contains("assistance_times")) {
    auto const f = cista::mmap{assistance_path.generic_string().c_str(),
                               cista::mmap::protection::READ};
    assistance = std::make_unique<assistance_times>(read_assistance(f.view()));
  }

  auto shapes = std::unique_ptr<shapes_storage>{};
  if (vm.contains("shapes")) {
    shapes = std::make_unique<shapes_storage>(out_shapes,
                                              cista::mmap::protection::WRITE);
  }

  auto const start = parse_date(start_date);
  load(input_files, finalize_opt, {start, start + date::days{n_days}},
       assistance.get(), shapes.get(), ignore && recursive)
      .write(out);

  auto ioc = boost::asio::io_context{};
  auto pool = boost::asio::io_context{};
  auto server = http_server{ioc, pool, opt.static_file_path_};

  server.listen(opt.http_host_, opt.http_port_);

  auto work_guard = boost::asio::make_work_guard(pool);
  auto threads = std::vector<std::thread>(std::max(1U, opt.threads_));
  for (auto& t : threads) {
    t = std::thread(run(pool));
  }

  server.listen(opt.http_host_, opt.http_port_);

  auto const stop = net::stop_handler(ioc, [&]() {
    server.stop();
    ioc.stop();
  });

  ioc.run();
  pool.stop();
  for (auto& t : threads) {
    t.join();
  }
}
