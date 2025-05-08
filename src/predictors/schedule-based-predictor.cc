#include "predictors/schedule-based-predictor.h"
#include <nigiri/timetable.h>
#include "predictors/predictor-utils.h"

#include <boost/geometry/algorithms/closest_points.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

// Typdeklarationen für bessere Lesbarkeit
using Point = boost::geometry::model::point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;
using Segment = boost::geometry::model::segment<Point>;

Point calculateFootPoint(const Point& vehicle_point, const Point& stop1_point, const Point& stop2_point) {
    // Erstelle ein Segment aus den zwei Haltestellen
    Segment stop_segment(stop1_point, stop2_point);
    
    // Erstelle einen Punkt für das Ergebnis
    Point foot_point;
    
    // Berechne den nächsten Punkt auf dem Segment
    boost::geometry::closest_points(stop_segment, vehicle_point, foot_point);
    
    return foot_point;
}

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

        // Fahrzeugposition als Boost Geometry Punkt
        Point vehicle_point;
        boost::geometry::set<0>(vehicle_point, vehicle_position.position().longitude());
        boost::geometry::set<1>(vehicle_point, vehicle_position.position().latitude());

        const std::string& tripID = vehicle_position.trip().trip_id();
        const auto& stops = predictorUtils::get_stops_for_trip(timetable, tripID);
        
        if (stops.size() < 2) {
            continue;
        }

        // Finde das nächste Segment
        size_t closest_segment_start = 0;
        double min_segment_distance = std::numeric_limits<double>::max();
        Point closest_foot_point;

        for (size_t i = 0; i < stops.size() - 1; ++i) {
            const auto& current_stop = stops[i];
            const auto& next_stop = stops[i + 1];

            // Erstelle Punkte für die Haltestellen
            Point stop1_point, stop2_point;
            boost::geometry::set<0>(stop1_point, current_stop.pos_.lng_);
            boost::geometry::set<1>(stop1_point, current_stop.pos_.lat_);
            boost::geometry::set<0>(stop2_point, next_stop.pos_.lng_);
            boost::geometry::set<1>(stop2_point, next_stop.pos_.lat_);

            // Berechne Lotfußpunkt für dieses Segment
            Point foot_point = calculateFootPoint(vehicle_point, stop1_point, stop2_point);

            // Berechne Distanz zum Lotfußpunkt
            double distance = boost::geometry::distance(
                vehicle_point, 
                foot_point,
                boost::geometry::strategy::distance::haversine<double>(6371000.0)  // Erdradius in Metern
            );

            if (distance < min_segment_distance) {
                min_segment_distance = distance;
                closest_segment_start = i;
                closest_foot_point = foot_point;
            }
        }

        // Hier haben wir:
        // 1. Die zwei relevanten Haltestellen: 
        //    stops[closest_segment_start] und stops[closest_segment_start + 1]
        // 2. Den Lotfußpunkt (closest_foot_point) mit
        
      double progress = boost::geometry::distance(stops[closest_segment_start], closest_foot_point, boost::geometry::strategy::distance::haversine(6371000.0)) /
        boost::geometry::distance(stops[closest_segment_start], stops[closest_segment_start + 1], boost::geometry::strategy::distance::haversine(6371000.0));

      // TODO: Time needed for the segment = Ankunftszeit B - Abfahrtszeit A

    }
}