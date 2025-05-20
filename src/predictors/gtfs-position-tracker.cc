#include "predictors/gtfs-position-tracker.h"
#include "predictors/predictor-utils.h"
#include <nigiri/timetable.h>
#include <chrono>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

GTFSPositionTracker::GTFSPositionTracker() = default;

/**
 * Predictor based on the GTFS-Position-tracker approach
 * This checks if any of the vehicle Positions is close to a stop. If so,
 * a tripUpdate for the according trip and stop will be created.
 *
 * @param tripUpdates current tripUpdate feed to be updated
 * @param vehiclePositions positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void GTFSPositionTracker::predict(
    transit_realtime::FeedMessage& tripUpdates,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
  
  //Save all current tripIds in vehiclePositions
  std::unordered_set<std::string> currentTripIDs = {};

  for (const transit_realtime::FeedEntity& entity : vehiclePositions.entity()) {
    // For this prototype we only care about vehicle positions. Service alerts and other trip updates are ignored
    if (entity.has_vehicle()) {
      const transit_realtime::VehiclePosition& vehicle_position = entity.vehicle();
      // Get Trip ID
      std::string tripID = vehicle_position.trip().trip_id();
      currentTripIDs.insert(tripID);

      const std::string routeID = vehicle_position.trip().route_id();
      const std::string vehicleID = vehicle_position.vehicle().id();

      // check for each stop if we are close
      for (nigiri::location location : predictorUtils::get_stops_for_trip(timetable, tripID)) {

        namespace bg = boost::geometry;
        bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>
            vehicle_point{};
        bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>
            location_point{};

        bg::set<0>(vehicle_point, vehicle_position.position().longitude());
        bg::set<1>(vehicle_point, vehicle_position.position().latitude());
        bg::set<0>(location_point, location.pos_.lng_);
        bg::set<1>(location_point, location.pos_.lat_);

        if (bg::distance(vehicle_point, location_point, bg::strategy::distance::haversine(6371000.0)) < 100) {
          auto now = std::chrono::system_clock::now();
          const auto current_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

          predictorUtils::set_trip_update(
              tripID,
              location.id_,
              vehicleID,
              routeID,
              current_time,
              tripUpdates);
        }
      }
    }
  }
  
  // Remove TripUpdates for trips not in vehiclePositions anymore
  predictorUtils::delete_old_trip_updates(currentTripIDs, tripUpdates);

  transit_realtime::FeedHeader* header = tripUpdates.mutable_header();
  header->set_timestamp(time(nullptr));
}
