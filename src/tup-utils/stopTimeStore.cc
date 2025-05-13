#include "tup-utils/stopTimeStore.h"

namespace fs = std::filesystem;

stopTimeStore::stopTimeStore(std::filesystem::path path,
                               cista::mmap::protection const mode)
    : mode_{mode},
      p_{[&]() {
        fs::create_directories(path);
        return std::move(path);
      }()},
      data_{mm_paged_vecvec_helper<stoptime_idx_t, stopTime>::data_t{
        mm_vec<stopTime>{mm("stoptimes_data.bin")}},
    mm_vec<cista::page<std::uint64_t, std::uint32_t>>{
      mm("stoptimes_idx.bin")}} {}

cista::mmap stopTimeStore::mm(char const* file) {
  return cista::mmap{(p_ / file).generic_string().c_str(), mode_};
}

void stopTimeStore::store(std::string const& trip_id,
                          std::string const& stop_id,
                          std::optional<int64_t> arrival_time,
                          std::optional<int64_t> departure_time,
                          stopTimeStore& storage) {
    stopTime new_stop_time{
        .trip_id = trip_id,
        .stop_id = stop_id,
        .arrival_time = arrival_time,
        .departure_time = departure_time
    };
    storage.data_.emplace_back({new_stop_time});
}