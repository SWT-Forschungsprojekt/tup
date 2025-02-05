#include <iostream>
#include <filesystem>
#include <thread>

#include "fmt/std.h"

#include "net/stop_handler.h"

#include "conf/options_parser.h"

#include "tup/backend/http_server.h"


namespace fs = std::filesystem;

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
