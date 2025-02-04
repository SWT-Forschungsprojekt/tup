#pragma once

#include "tag_lookup.h"

#include <memory>

#include "cista/memory_holder.h"

#include "date/date.h"

#include "nigiri/types.h"

#include "tup/config.h"

#include <nigiri/shapes_storage.h>
#include <nigiri/timetable.h>

namespace tup {

struct elevators;

template <typename T>
struct point_rtree;

template <typename T>
using ptr = std::unique_ptr<T>;

struct data {
  data(std::filesystem::path);
  data(std::filesystem::path, config const&);
  ~data();

  data(data const&) = delete;
  data& operator=(data const&) = delete;

  data(data&&);
  data& operator=(data&&);

  friend std::ostream& operator<<(std::ostream&, data const&);

  void load_osr();
  void load_tt(std::filesystem::path const&);
  void load_shapes();
  void load_railviz();
  void load_geocoder();
  void load_matches();
  void load_reverse_geocoder();
  void load_elevators();
  void load_tiles();

  void init_rtt(date::sys_days = std::chrono::time_point_cast<date::days>(
                    std::chrono::system_clock::now()));

  auto cista_members() {
    // !!! Remember to add all new members !!!
    return std::tie(tt_, tags_, location_rtee_, shapes_);
  }

  std::filesystem::path path_;
  config config_;

  cista::wrapped<nigiri::timetable> tt_;
  cista::wrapped<tag_lookup> tags_;
  ptr<point_rtree<nigiri::location_idx_t>> location_rtee_;
  ptr<nigiri::shapes_storage> shapes_;
};

}  // namespace tup
