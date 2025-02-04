#include "tup/data.h"

#include <filesystem>
#include <future>

#include "tup/tt_location_rtree.h"

namespace fs = std::filesystem;
namespace n = nigiri;

namespace tup {

  data::~data() = default;
  data::data(data&&) = default;
  data& data::operator=(data&&) = default;

  void data::load_tt(fs::path const& p) {
    tags_ = tag_lookup::read(path_ / "tags.bin");
    tt_ = n::timetable::read(path_ / p);
    tt_->locations_.resolve_timezones();
    location_rtee_ = std::make_unique<point_rtree<n::location_idx_t>>(
        create_location_rtree(*tt_));
    init_rtt();
  }
}// namespace tup
