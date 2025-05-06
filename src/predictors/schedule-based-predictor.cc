#include "predictors/schedule-based-predictor.h"
#include "predictors/predictor-utils.h"
#include <nigiri/timetable.h>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>


ScheduleBasedPredictor::ScheduleBasedPredictor() = default;

/**
 * Predictor based on a given schedule
 * This checks where the vehicle currently is in the schedule and checks if it
 * is too late. If so, it interpolates how much time the vehicle will be too
 * late at the next stop
 * @param outputFeed current tripUpdate feed to be updated
 * @param vehiclePositionFeed positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void ScheduleBasedPredictor::predict(
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
        // check for each stop if it is the closest one
        nigiri::location& closest_stop = stop_list[0];
        double distance = 10000000;
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

          const auto current_distance = bg::distance(vehicle_point, location_point, bg::strategy::distance::haversine(6371000.0));
          if (current_distance < distance) {
            distance = current_distance;
            closest_stop = location;
          }
        }

      }
    }
}
