#include "predictors/gtfs-position-tracker.h"
#include <nigiri/timetable.h>

GTFSPositionTracker::GTFSPositionTracker() = default;

void GTFSPositionTracker::predict(
    transit_realtime::FeedMessage& message,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
    for (int i = 0; i < vehiclePositions.entity_size(); i++) {
      transit_realtime::FeedEntity* entity = message.mutable_entity(i);
      if (entity->has_vehicle()) {
        transit_realtime::VehiclePosition* vehicle_position = entity->mutable_vehicle();
        // Get Trip ID
        // Get a stop list for a given trip
        // check for each stop if we are close
        // if we are close, check if a corresponding tripUpdate exists
        // if not, create a new one
        // if yes, update it accordingly
      }
    }};
