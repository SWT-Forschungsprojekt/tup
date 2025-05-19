#include "predictors/historic-average-predictor.h"
#include "predictors/predictor-utils.h"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

HistoricAveragePredictor::HistoricAveragePredictor() = default;

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
  // Part 1: Storing of departures based on GTFS-Position-Tracker
  auto now = std::chrono::system_clock::now();
  auto current_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  auto today = std::format("{:%Y-%m-%d}", now);

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
          this->store_.store(tripID, location.id_.data(), current_time, today);
        }
      }
    }
  }
  // Part 2: Prediction based on the stored departures/arrivals
  // Empty output feed, as we generate all tripUpdates every time new
  tripUpdates.Clear();
  transit_realtime::FeedHeader* header = tripUpdates.mutable_header();
  header->set_gtfs_realtime_version("2.0");
  header->set_incrementality(transit_realtime::FeedHeader_Incrementality_FULL_DATASET);
  header->set_timestamp(time(nullptr));
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
      auto old_distance = std::numeric_limits<double>::max();
      nigiri::location& prediction_stop = stop_list.back();
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

        double new_distance = bg::distance(vehicle_point, location_point, bg::strategy::distance::haversine(6371000.0));
        if (old_distance > new_distance) {
          prediction_stop = location;
          break;
        }
      }
      auto arrival_time = this->store_.getAverageArrivalTime(tripID, prediction_stop.id_.data());
      if (arrival_time == 0) {
        continue;
      }
      // Create a prediction and add it to outputFeed
      transit_realtime::FeedEntity* new_entity = tripUpdates.add_entity();
      new_entity->set_id(tripID);

      transit_realtime::TripUpdate* tripUpdateToUpdate = new_entity->mutable_trip_update();
      transit_realtime::TripDescriptor* trip = tripUpdateToUpdate->mutable_trip();
      trip->set_trip_id(tripID);
      trip->set_route_id(routeID);
      tripUpdateToUpdate->mutable_vehicle()->set_id(vehicleID);

      transit_realtime::TripUpdate_StopTimeUpdate* stop_time_update = tripUpdateToUpdate->add_stop_time_update();
      stop_time_update->set_stop_id(prediction_stop.id_);
      transit_realtime::TripUpdate_StopTimeEvent* departureToUpdate = stop_time_update->mutable_departure();

      tripUpdateToUpdate->set_timestamp(current_time);
      departureToUpdate->set_time(arrival_time);
    }
  }
}

/**
 * Helping method to load historic data from protobuf files
 * @param stopTimes vector of stopTimes to load
 */
void HistoricAveragePredictor::loadHistoricData(const std::vector<stopTime>& stopTimes) {
  const auto t = std::time(nullptr);
  const auto tm = *std::localtime(&t);
  for (stopTime stopTime : stopTimes) {
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    this->store_.store(stopTime.trip_id, stopTime.stop_id, stopTime.arrival_time, oss.str());
  }
}
