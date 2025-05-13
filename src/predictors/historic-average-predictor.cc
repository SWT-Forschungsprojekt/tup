#include "predictors/historic-average-predictor.h"
#include "predictors/predictor-utils.h"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

/**
 * Predictor based on the HistoricAveragePredictor approach
 * This checks when the trip reached the following trip in the last n trips
 * and uses this as a prediction.
 *
 * @param tripUpdates current tripUpdate feed to be updated
 * @param vehiclePositions positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void HistoricAveragePredictor::predict(
    transit_realtime::FeedMessage& tripUpdates,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
  // TODO: Implement HistoricAveragePredictor
  // Part 1: Storing of departures based on GTFS-Position-Tracker
  for (const transit_realtime::FeedEntity& entity : vehiclePositions.entity()) {
    // For this prototype we only care about vehicle positions. Service alerts and other trip updates are ignored
    if (entity.has_vehicle()) {
      const transit_realtime::VehiclePosition& vehicle_position = entity.vehicle();
      // Get Trip ID
      std::string tripID = vehicle_position.trip().trip_id();
      std::string routeID = vehicle_position.trip().route_id();
      std::string vehicleID = vehicle_position.vehicle().id();

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
          this->store_.getAverageArrivalTime(tripID, location.id_.data(), this->store_);
        }
      }
    }
  }
  // Part 2: Prediction based on the stored departures/arrivals
  for (const transit_realtime::FeedEntity& entity : vehiclePositions.entity()) {
    // For this prototype we only care about vehicle positions. Service alerts and other trip updates are ignored
    if (entity.has_vehicle()) {
      const transit_realtime::VehiclePosition& vehicle_position = entity.vehicle();
      // Get Trip ID
      std::string tripID = vehicle_position.trip().trip_id();
      std::string routeID = vehicle_position.trip().route_id();
      std::string vehicleID = vehicle_position.vehicle().id();

      // Get a stop list for a given trip
      std::vector<nigiri::location> stop_list = predictorUtils::get_stops_for_trip(timetable, tripID);
      // TODO: Check for each stop if we before or after the stop. Store the first one where we are before.
      for (nigiri::location location : stop_list) {

      }
    }
  }
}

/**
 * Helping method to load historic data from protobuf files
 * @param stopTimes vector of stopTimes to load
 */
void HistoricAveragePredictor::loadHistoricData(std::vector<stopTime> stopTimes) {
  for (auto stopTime : stopTimes) {
    this->store_.store(stopTime.trip_id, stopTime.stop_id, stopTime.arrival_time, stopTime.departure_time, this->store_);
  }
}
