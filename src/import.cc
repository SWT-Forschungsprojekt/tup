#include "tup/import.h"

#include <utl/progress_tracker.h>

#include <fstream>
#include <ostream>
#include <vector>

#include "cista/io.h"

#include "utl/erase_if.h"
#include "utl/to_vec.h"

#include "nigiri/loader/assistance.h"
#include "nigiri/loader/load.h"
#include "nigiri/loader/loader_interface.h"
#include "nigiri/clasz.h"
#include "nigiri/common/parse_date.h"
#include "nigiri/shapes_storage.h"
#include "nigiri/timetable.h"

#include "tup/clog_redirect.h"
#include "tup/data.h"
#include "tup/hashes.h"
#include "tup/tag_lookup.h"
#include "tup/tt_location_rtree.h"

namespace fs = std::filesystem;
namespace n = nigiri;
namespace nl = nigiri::loader;
using namespace std::string_literals;

namespace fs = std::filesystem;

namespace tup {

struct task {
  friend std::ostream& operator<<(std::ostream& out, task const& t) {
    return out << t.name_;
  }

  bool ready_for_load(fs::path const& data_path) {
    auto const existing = read_hashes(data_path, name_);
    if (existing != hashes_) {
      std::cout << name_ << "\n"
                << "  existing: " << to_str(existing) << "\n"
                << "  current: " << to_str(hashes_) << "\n";
    }
    return existing == hashes_;
  }

  void load() { load_(); }

  void run(fs::path const& data_path) {
    auto const pt = utl::activate_progress_tracker(name_);
    auto const redirect = clog_redirect{
      (data_path / "logs" / (name_ + ".txt")).generic_string().c_str()};
    run_();
    write_hashes(data_path, name_, hashes_);
    pt->out_ = 100;
    pt->status("FINISHED");
  }

  std::string name_;
  std::function<bool()> should_run_;
  std::function<bool()> ready_;
  std::function<void()> run_;
  std::function<void()> load_;
  meta_t hashes_;
  utl::progress_tracker_ptr pt_{};
};

}  // namespace tup

template <>
struct fmt::formatter<tup::task> : fmt::ostream_formatter {};

namespace tup {

cista::hash_t hash_file(fs::path const& p) {
  if (p.generic_string().starts_with("\n#")) {
    return cista::hash(p.generic_string());
  } else if (fs::is_directory(p)) {
    auto h = cista::BASE_HASH;
    for (auto const& file : fs::directory_iterator{p}) {
      h = cista::hash_combine(h, hash_file(file));
    }
    return h;
  } else {
    auto const mmap =
        cista::mmap{p.generic_string().c_str(), cista::mmap::protection::READ};
    return cista::hash_combine(
        cista::hash(mmap.view().substr(
            0U, std::min(mmap.size(),
                         static_cast<std::size_t>(50U * 1024U * 1024U)))),
        mmap.size());
  }
}

data import(config const& c, fs::path const& data_path, bool const write) {
  c.verify_input_files_exist();

  auto ec = std::error_code{};
  create_directories(data_path / "logs", ec);
  create_directories(data_path / "meta", ec);
  {
    auto cfg = std::ofstream{(data_path / "config.yml").generic_string()};
    cfg.exceptions(std::ios_base::badbit | std::ios_base::eofbit);
    cfg << c << "\n";
    cfg.close();
  }

  clog_redirect::set_enabled(write);

  auto tt_hash = std::pair{"timetable"s, cista::BASE_HASH};
  if (c.timetable_.has_value()) {
    auto& h = tt_hash.second;
    auto const& t = *c.timetable_;

    for (auto const& [_, d] : t.datasets_) {
      h = cista::build_hash(h, c.osr_footpath_, hash_file(d.path_),
                            d.default_bikes_allowed_, d.clasz_bikes_allowed_,
                            d.rt_, d.default_timezone_);
    }

    h = cista::build_hash(
        h, t.first_day_, t.num_days_, t.with_shapes_, t.ignore_errors_,
        t.adjust_footpaths_, t.merge_dupes_intra_src_, t.merge_dupes_inter_src_,
        t.link_stop_distance_, t.update_interval_, t.incremental_rt_update_,
        t.max_footpath_length_, t.default_timezone_, t.assistance_times_);
  }

  auto d = tup::data{data_path};

  auto tt = task{
    "tt",
    [&]() { return c.timetable_.has_value(); },
    [&]() { return true; },
    [&]() {
      auto const to_clasz_bool_array =
          [&](tup::config::timetable::dataset const& d) {
            auto a = std::array<bool, n::kNumClasses>{};
            a.fill(d.default_bikes_allowed_);
            if (d.clasz_bikes_allowed_.has_value()) {
              for (auto const& [clasz, allowed] : *d.clasz_bikes_allowed_) {
                a[static_cast<unsigned>(n::to_clasz(clasz))] = allowed;
              }
            }
            return a;
      };

      auto const& t = *c.timetable_;

      auto const first_day = n::parse_date(t.first_day_);
      auto const interval = n::interval<date::sys_days>{
        first_day, first_day + std::chrono::days{t.num_days_}};

      auto assistance = std::unique_ptr<nl::assistance_times>{};
      if (t.assistance_times_.has_value()) {
        auto const f =
            cista::mmap{t.assistance_times_->generic_string().c_str(),
                        cista::mmap::protection::READ};
        assistance = std::make_unique<nl::assistance_times>(
            nl::read_assistance(f.view()));
      }

      if (t.with_shapes_) {
        d.shapes_ = std::make_unique<n::shapes_storage>(
            data_path, cista::mmap::protection::WRITE);
      }

      d.tt_ = cista::wrapped{cista::raw::make_unique<n::timetable>(nl::load(
          utl::to_vec(
              t.datasets_,
              [&, src = n::source_idx_t{}](auto&& x) mutable
              -> std::pair<std::string, nl::loader_config> {
                auto const& [tag, dc] = x;
                return {dc.path_,
                        {
                            .link_stop_distance_ = t.link_stop_distance_,
                            .default_tz_ = dc.default_timezone_.value_or(
                                dc.default_timezone_.value_or("")),
                            .bikes_allowed_default_ = to_clasz_bool_array(dc),
                        }};
              }),
          {.adjust_footpaths_ = t.adjust_footpaths_,
           .merge_dupes_intra_src_ = t.merge_dupes_intra_src_,
           .merge_dupes_inter_src_ = t.merge_dupes_inter_src_,
           .max_footpath_length_ = t.max_footpath_length_},
          interval, assistance.get(), d.shapes_.get(), t.ignore_errors_))};
      d.location_rtee_ =
          std::make_unique<point_rtree<nigiri::location_idx_t>>(
              create_location_rtree(*d.tt_));

      if (write) {
        d.tt_->write(data_path / "tt.bin");
      }

      d.init_rtt();

      if (c.timetable_->with_shapes_) {
        d.load_shapes();
      }
    },
    [&]() {
      d.load_tt("tt.bin");
      if (c.timetable_->with_shapes_) {
        d.load_shapes();
      }
    },
    {tt_hash, n_version()}};


  auto tasks =
      std::vector<task>{tt};
  utl::erase_if(tasks, [&](auto&& t) {
    if (!t.should_run_()) {
      return true;
    }

    if (t.ready_for_load(data_path)) {
      t.load();
      return true;
    }

    return false;
  });

  for (auto& t : tasks) {
    t.pt_ = utl::activate_progress_tracker(t.name_);
  }

  while (!tasks.empty()) {
    auto const task_it =
        utl::find_if(tasks, [](task const& t) { return t.ready_(); });
    utl::verify(task_it != end(tasks), "no task to run, remaining tasks: {}",
                tasks);
    task_it->run(data_path);
    tasks.erase(task_it);
  }

  return d;
}
} // using namespace tup
