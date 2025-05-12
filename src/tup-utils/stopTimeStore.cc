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
      mm("shapes_idx.bin")}} {}

cista::mmap stopTimeStore::mm(char const* file) {
  return cista::mmap{(p_ / file).generic_string().c_str(), mode_};
}
