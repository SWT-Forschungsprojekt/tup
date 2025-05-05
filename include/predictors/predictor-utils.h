#pragma once
#include <nigiri/timetable.h>

class predictorUtils {
  public:
    static std::vector<nigiri::location> get_stops_for_trip(nigiri::timetable const& timetable, std::string const& trip_id);
    static auto convert_trip_id_to_idx(nigiri::timetable const& timetable, std::string const& trip_id) -> nigiri::trip_idx_t;
};
