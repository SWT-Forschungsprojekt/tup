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
    transit_realtime::FeedMessage& outputFeed,
    const transit_realtime::FeedMessage& vehiclePositionFeed,
    const nigiri::timetable& timetable) {
    for (const transit_realtime::FeedEntity& entity : vehiclePositionFeed.entity()) {
      // For this prototype we only care about vehicle positions. Service alerts and other trip updates are ignored
      if (entity.has_vehicle()) {
        const transit_realtime::VehiclePosition& vehicle_position = entity.vehicle();
        // Get Trip ID
        std::string tripID = vehicle_position.trip().trip_id();
        // Get a stop list for a given trip
        std::vector<nigiri::location> stop_list = predictorUtils::get_stops_for_trip(timetable, tripID);
        // check for each stop if we are close
        for (nigiri::location location : stop_list) {

          namespace bg = boost::geometry;
          bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>
              vehicle_point{};
          bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>
              location_point{};

          bg::set<0>(vehicle_point, vehicle_position.position().longitude());
          bg::set<1>(vehicle_point, vehicle_position.position().latitude());
          bg::set<0>(location_point, location.pos_.lng_);
          bg::set<1>(location_point, location.pos_.lat_);

          const auto distance = bg::distance(vehicle_point, location_point, bg::strategy::distance::haversine(6371000.0));

          if (distance < 100) {
            transit_realtime::TripUpdate_StopTimeUpdate stopTimeUpdate;
            bool tripUpdateExists = false;

            for (const transit_realtime::FeedEntity& outputFeedEntity : outputFeed.entity()) {
              if (outputFeedEntity.has_trip_update() &&
                  outputFeedEntity.trip_update().trip().trip_id() == tripID) {
                for (const transit_realtime::TripUpdate_StopTimeUpdate& update : outputFeedEntity.trip_update().stop_time_update()) {
                  if (update.stop_id() == location.id_) {
                    tripUpdateExists = true;
                    stopTimeUpdate = update;
                    break;
                  }
                }
              }
            }

            auto now = std::chrono::system_clock::now();
            auto current_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            
            if (tripUpdateExists) {
              stopTimeUpdate.mutable_departure()->set_time(std::max(current_time, stopTimeUpdate.departure().time()));
            } else {
              transit_realtime::FeedEntity* new_entity = outputFeed.add_entity();
              new_entity->set_id(tripID);

              transit_realtime::TripUpdate* trip_update = new_entity->mutable_trip_update();
              trip_update->mutable_trip()->set_trip_id(tripID);

              transit_realtime::TripUpdate_StopTimeUpdate* stop_time_update = trip_update->add_stop_time_update();
              stop_time_update->set_stop_id(location.id_);
              stop_time_update->mutable_departure()->set_time(current_time);

              std::cerr << "Create trip update" << timetable.locations_.ids_.size() << std::endl;
            }

          }
        }
      }
    }};
