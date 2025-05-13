#pragma once

#include <cista/containers/mmap_vec.h>
#include <cista/containers/paged.h>
#include <cista/containers/paged_vecvec.h>
#include <cista/mmap.h>
#include <cista/strong.h>

#include <filesystem>
#include <optional>
#include <string>

using stoptime_idx_t = cista::strong<std::uint32_t, struct _shape_idx>;

struct stopTime {
  stoptime_idx_t idx;
  std::string trip_id;
  std::string stop_id;
  std::optional<int64_t> arrival_time;
  std::optional<int64_t> departure_time;
};

template <typename T>
using mm_vec = cista::basic_mmap_vec<T, std::uint64_t>;

template <typename Key, typename T>
struct mm_paged_vecvec_helper {
  using data_t =
      cista::paged<mm_vec<T>, std::uint64_t, std::uint32_t, 2U, 1U << 31U>;
  using idx_t = mm_vec<typename data_t::page_t>;
  using type = cista::paged_vecvec<idx_t, data_t, Key>;
};

template <typename Key, typename T>
using mm_paged_vecvec = typename mm_paged_vecvec_helper<Key, T>::type;

class stopTimeStore {
  stopTimeStore(std::filesystem::path, cista::mmap::protection);
public:
  cista::mmap mm(char const* file);

  cista::mmap::protection mode_;
  std::filesystem::path p_;

  mm_paged_vecvec<stoptime_idx_t, stopTime> data_{};

  static void store(std::string const& trip_id,
                    std::string const& stop_id,
                    std::optional<int64_t> arrival_time,
                    std::optional<int64_t> departure_time,
                    stopTimeStore& storage);
  static int64_t getAverageArrivalTime(std::string const& trip_id, std::string const& stop_id);
};
