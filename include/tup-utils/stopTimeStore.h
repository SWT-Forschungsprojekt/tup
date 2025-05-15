#pragma once

#include <cista/containers/mmap_vec.h>
#include <cista/containers/paged.h>
#include <cista/containers/paged_vecvec.h>
#include <cista/mmap.h>
#include <cista/strong.h>

#include <filesystem>
#include <optional>
#include <string>

using stop_time_idx_t = cista::strong<std::uint32_t, struct stoptime_idx>;

/**
 * An Event when a vehicle of a trip arrives or ends at a stop
 */
struct stopTime {
  stop_time_idx_t idx;
  std::string trip_id;
  std::string stop_id;
  std::optional<int64_t> arrival_time;
  std::optional<int64_t> departure_time;
  std::string date;
};

template <typename T>
using mm_vec = cista::basic_mmap_vec<T, std::uint64_t>;

template <typename Key, typename T>
/**
 * Helper to deal with the memory-mapped paged double vector
 */
struct mm_paged_vecvec_helper {
  using data_t =
      cista::paged<mm_vec<T>, std::uint64_t, std::uint32_t, 2U, 1U << 31U>;
  using idx_t = mm_vec<typename data_t::page_t>;
  using type = cista::paged_vecvec<idx_t, data_t, Key>;
};

template <typename Key, typename T>
using mm_paged_vecvec = typename mm_paged_vecvec_helper<Key, T>::type;

/**
 * Stores historic stopTimes which are then used for prediction
 */
class stopTimeStore {
  /**
   * Create a stopTimeStore storing the memory-mapped files
   */
  mm_paged_vecvec<stop_time_idx_t, stopTime> data_;
public:
  stopTimeStore(std::filesystem::path, cista::mmap::protection);
  cista::mmap mm(char const* file);

  cista::mmap::protection mode_;
  std::filesystem::path p_;

  static void store(std::string const& trip_id,
                    std::string const& stop_id,
                    std::optional<int64_t> arrival_time,
                    std::optional<int64_t> departure_time,
                    stopTimeStore& storage);
  static int64_t getAverageArrivalTime(std::string const& trip_id, std::string const& stop_id, stopTimeStore& storage);
};
