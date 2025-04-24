#pragma once
#include <nigiri/timetable.h>

#include "gtfs-rt/gtfs-realtime.pb.h"

class GTFSPositionTracker {
  public:
    GTFSPositionTracker();

  // Method to predict the next stop and arrival time
  static void predict(transit_realtime::FeedMessage& message, transit_realtime::FeedMessage& vehiclePositions,
                        const nigiri::timetable& timetable);
};