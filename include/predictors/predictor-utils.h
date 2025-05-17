#pragma once
#include <nigiri/timetable.h>
#include <unordered_set>
#include "gtfs-rt/gtfs-realtime.pb.h"

class predictorUtils {
  public:
    static std::vector<nigiri::location> get_stops_for_trip(nigiri::timetable const& timetable, std::string const& trip_id);
    static auto convert_trip_id_to_idx(nigiri::timetable const& timetable, std::string const& trip_id) -> nigiri::trip_idx_t;
    static void delete_old_trip_updates(std::unordered_set<std::string> currentTripIDs, transit_realtime::FeedMessage& outputFeed);
    static void set_trip_update(std::string tripID, std::string_view stopID, std::string vehicleID, std::string routeID, int64_t newArivalTime, transit_realtime::FeedMessage& outputFeed);
};
