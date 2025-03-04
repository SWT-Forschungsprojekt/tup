#include <iostream>
#include <filesystem>
#include <thread>

#include "fmt/std.h"

#include "net/stop_handler.h"

#include "tup/backend/http_server.h"

#include <vector>

#include "boost/algorithm/string.hpp"
#include "boost/program_options.hpp"

#include "date/date.h"

#include <string>
#include <curl/curl.h>
#ifdef NO_DATA
#undef NO_DATA
#endif
#include "gtfsrt/gtfs-realtime.pb.h"

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

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

bool DownloadProtobuf(const std::string& url, std::string& out_data) {
  CURL* curl;
  CURLcode res;
  curl = curl_easy_init();
  if (!curl) return false;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_data);
  res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK);
}

int main(int argc, char const* argv[]) {
  std::cout << "Hello, World!" << std::endl;

  auto const progress_tracker = utl::activate_progress_tracker("importer");
  auto const silencer = utl::global_progress_bars{true};

  auto config_file = fs::path{"config.yaml"};
  auto data_dir = fs::path{"tup"};
  auto http_host = "0.0.0.0"s;
  auto http_port = "8000"s;
  auto static_file_path = fs::path{};
  unsigned threads_ = std::thread::hardware_concurrency();
  bool lock = true;

  std::string vehicle_position_url;

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

      ("config,c", bpo::value(&config_file)->default_value(config_file), "config file path")  //
      ("data,d", bpo::value(&data_dir)->default_value(data_dir), "data directory")  //
      ("host", bpo::value(&http_host)->default_value(http_host), "HTTP host")  //
      ("port,p", bpo::value(&http_port)->default_value(http_port), "HTTP port")  //
      ("static", bpo::value(&static_file_path)->default_value(static_file_path), "Path to static files (ui/web)")  //
      ("threads,t", bpo::value(&threads_)->default_value(threads_), "Number of routing threads")  //
      ("lock,l", bpo::bool_switch(&lock)->default_value(lock), "Lock to memory")  //

      // e.g. http://realtime.prod.obahart.org:8088/vehicle-positions
      ("vehicle_positions_url,v", bpo::value(&vehicle_position_url)->required(), "URL for vehicle positions")  //

      ("in,i", bpo::value(&in)->required(), "input path")  //
      ("recursive,r", bpo::bool_switch(&recursive)->default_value(false),
       "read all zips and directories from the input directory")  //
      ("ignore", bpo::bool_switch(&ignore)->default_value(false),
       "ignore if a directory entry is not a timetable (only for recursive)")  //
      ("out,o", bpo::value(&out)->default_value(out), "output file path")  //
      ("start_date,s", bpo::value(&start_date)->default_value(start_date),
       "start date of the timetable, format: YYYY-MM-DD")  //
      ("num_days,n", bpo::value(&n_days)->default_value(n_days),
       "the length of the timetable in days")  //
      ("tz", bpo::value(&c.default_tz_)->default_value(c.default_tz_),
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
  auto server = http_server{ioc, pool, static_file_path};

  auto work_guard = boost::asio::make_work_guard(pool);
  auto threads = std::vector<std::thread>(std::max(1U, threads_));
  for (auto& t : threads) {
    t = std::thread(run(pool));
  }

  std::string protobuf_data;

  if (!DownloadProtobuf(vehicle_position_url, protobuf_data)) {
    std::cerr << "Fehler beim Herunterladen der Protobuf-Datei" << std::endl;
    return 1;
  }

  transit_realtime::FeedMessage feed;
  if (!feed.ParseFromString(protobuf_data)) {
    std::cerr << "Fehler beim Parsen der Protobuf-Daten" << std::endl;
    return 1;
  }

  std::cout << "Success" << std::endl;

  server.listen(http_host, http_port);

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
