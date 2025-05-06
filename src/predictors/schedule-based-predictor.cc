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
    if (!entity.has_vehicle()) {
      continue;
    }

    const auto& vehicle_position = entity.vehicle();
    if (!vehicle_position.has_trip() || !vehicle_position.has_position()) {
      continue;
    }

    const std::string& tripID = vehicle_position.trip().trip_id();
    const auto& stops = predictorUtils::get_stops_for_trip(timetable, tripID);
    
    if (stops.size() < 2) {
      continue;  // Brauchen mindestens zwei Haltestellen für eine Strecke
    }

    // Fahrzeugposition als Geometry Punkt
    namespace bg = boost::geometry;
    bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>> vehicle_point{};
    bg::set<0>(vehicle_point, vehicle_position.position().longitude());
    bg::set<1>(vehicle_point, vehicle_position.position().latitude());

    // Finde die zwei nächstgelegenen aufeinanderfolgenden Haltestellen
    size_t closest_segment_start = 0;
    double min_segment_distance = std::numeric_limits<double>::max();

    for (size_t i = 0; i < stops.size() - 1; ++i) {
      const auto& current_stop = stops[i];
      const auto& next_stop = stops[i + 1];

      // Erstelle Punkte für aktuelle und nächste Haltestelle
      bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>> 
          stop1_point{}, stop2_point{};
      
      bg::set<0>(stop1_point, current_stop.pos_.lng_);
      bg::set<1>(stop1_point, current_stop.pos_.lat_);
      bg::set<0>(stop2_point, next_stop.pos_.lng_);
      bg::set<1>(stop2_point, next_stop.pos_.lat_);

      // Berechne Abstand zu beiden Haltestellen
      double dist1 = bg::distance(vehicle_point, stop1_point, 
          bg::strategy::distance::haversine<double>(6371000.0));  // Erdradius in Metern
      double dist2 = bg::distance(vehicle_point, stop2_point,
          bg::strategy::distance::haversine<double>(6371000.0));

      // Berechne gewichtete Distanz zum Segment
      double segment_distance = std::min(dist1, dist2);
      
      if (segment_distance < min_segment_distance) {
        min_segment_distance = segment_distance;
        closest_segment_start = i;
      }
    }

    // Hier haben wir die zwei relevanten Haltestellen gefunden:
    // stops[closest_segment_start] und stops[closest_segment_start + 1]
    
    // Hier könnte weitere Logik für die Prädiktion folgen …
  }
}