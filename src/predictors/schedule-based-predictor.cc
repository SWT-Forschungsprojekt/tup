#include "predictors/schedule-based-predictor.h"
#include <nigiri/timetable.h>
#include "predictors/predictor-utils.h"

#include <boost/geometry/algorithms/closest_points.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

// central type declarations
using Point = boost::geometry::model::point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>>;
using Segment = boost::geometry::model::segment<Point>;

/**
 * Calculates the foot (nearest) point on a segment defined by two stops to a
 * given vehicle point. This function determines the point on the segment
 * closest to the provided vehicle position, commonly used for determining
 * vehicle alignment on a route.
 *
 * @param vehicle_point the point representing the current vehicle position.
 * @param stop1_point the point representing the first stop of the segment.
 * @param stop2_point the point representing the second stop of the segment.
 * @return the nearest point on the segment to the vehicle point.
 */
Point calculateFootPoint(const Point& vehicle_point, const Point& stop1_point, const Point& stop2_point) {
    // create a segment from to stops
    const Segment stop_segment(stop1_point, stop2_point);
    
    // Create a segment for the result
    Segment result_segment;
    
    // Closest-Point on a segment
    boost::geometry::closest_points(stop_segment, vehicle_point, result_segment);
    
    // return the first point of a segment
    return result_segment.first;
}

ScheduleBasedPredictor::ScheduleBasedPredictor() = default;

/**
 * Predictor based on a given schedule
 * This checks where the vehicle currently is in the schedule and checks if it
 * is too late. If so, it interpolates how much time the vehicle will be too
 * late at the next stop
 * @param outputFeed current tripUpdate feed to be updated
 * @param vehiclePositions positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void ScheduleBasedPredictor::predict(
    transit_realtime::FeedMessage& outputFeed,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
    
    for (const transit_realtime::FeedEntity& entity : vehiclePositions.entity()) {
        if (!entity.has_vehicle()) {
            continue;
        }

        const auto& vehicle_position = entity.vehicle();
        if (!vehicle_position.has_trip() || !vehicle_position.has_position()) {
            continue;
        }

        // vehicle position as Boost Geometry Punkt
        Point vehicle_point;
        boost::geometry::set<0>(vehicle_point, vehicle_position.position().longitude());
        boost::geometry::set<1>(vehicle_point, vehicle_position.position().latitude());

        const std::string& tripID = vehicle_position.trip().trip_id();
        const auto& stops = predictorUtils::get_stops_for_trip(timetable, tripID);
        
        if (stops.size() < 2) {
            continue;
        }

        // find the closest segment to the vehicle position
        size_t closest_segment_start = 0;
        double min_segment_distance = std::numeric_limits<double>::max();
        Point closest_foot_point;

        for (size_t i = 0; i < stops.size() - 1; ++i) {
            const auto& current_stop = stops[i];
            const auto& next_stop = stops[i + 1];

            // Create points for the two stops
            Point stop1_point, stop2_point;
            boost::geometry::set<0>(stop1_point, current_stop.pos_.lng_);
            boost::geometry::set<1>(stop1_point, current_stop.pos_.lat_);
            boost::geometry::set<0>(stop2_point, next_stop.pos_.lng_);
            boost::geometry::set<1>(stop2_point, next_stop.pos_.lat_);

            // Calculate the foot point on the segment between the two stops
            Point foot_point = calculateFootPoint(vehicle_point, stop1_point, stop2_point);

            // calculate the distance between the vehicle point and the foot point
            const double distance = boost::geometry::distance(
                vehicle_point, 
                foot_point,
                boost::geometry::strategy::distance::haversine(6371000.0)
            );

            if (distance < min_segment_distance) {
                min_segment_distance = distance;
                closest_segment_start = i;
                closest_foot_point = foot_point;
            }
        }
      Point segment_start, segment_end;
      boost::geometry::set<0>(segment_start, stops[closest_segment_start].pos_.lng_);
      boost::geometry::set<1>(segment_start, stops[closest_segment_start].pos_.lat_);
      boost::geometry::set<0>(segment_end, stops[closest_segment_start + 1].pos_.lng_);
      boost::geometry::set<1>(segment_end,
                              stops[closest_segment_start + 1].pos_.lat_);
      const double progress_way = boost::geometry::distance(segment_start, closest_foot_point, boost::geometry::strategy::distance::haversine(6371000.0)) /
        boost::geometry::distance(segment_start, segment_end, boost::geometry::strategy::distance::haversine(6371000.0));
      // progress_time: (current_time - a.departure_time) / (b.arrival_time - a.departure_time)
      auto now = std::chrono::system_clock::now();
      const auto current_time =
          std::chrono::duration_cast<std::chrono::seconds>(
              now.time_since_epoch())
              .count();
      const auto trip_idx = predictorUtils::convert_trip_id_to_idx(timetable, tripID);
      const auto& transport_ranges = timetable.trip_transport_ranges_;
      const auto t_idx = transport_ranges[trip_idx][0].first; // Beachten Sie .from statt .from_
      // convert to sys_days
      const auto today = date::floor<date::days>(now);
      const nigiri::day_idx_t current_day_idx = timetable.day_idx(today);
      auto departure_time = timetable.event_time(
          nigiri::transport{
              t_idx,
              current_day_idx
          },
          // Convert from `location_idx_t` to `stop_idx_t`
          nigiri::stop_idx_t{static_cast<unsigned short int>(
              static_cast<unsigned>(closest_segment_start))},
          nigiri::event_type::kDep
      );
      auto arrival_time = timetable.event_time(
          nigiri::transport{t_idx, current_day_idx},
          // Convert from `location_idx_t` to `stop_idx_t`
          nigiri::stop_idx_t{
              static_cast<unsigned short int>(static_cast<unsigned>(
                  closest_segment_start + 1))},
          nigiri::event_type::kDep);
      const double progress_time = (current_time - departure_time.time_since_epoch().count()) / (arrival_time.time_since_epoch().count() - departure_time.time_since_epoch().count());
      // too_late = progress_time > progress_way
      if (progress_time > progress_way) {
        // Calculate predicted arrival: current_time + progress_time * (1 /
        // progress_way)
        const auto predicted_arrival = current_time + static_cast<int>(progress_time * (1 / progress_way));
        // Update the feed accordingly
        transit_realtime::TripUpdate_StopTimeUpdate stopTimeUpdate;
        bool tripUpdateExists = false;

        for (const transit_realtime::FeedEntity& outputFeedEntity : outputFeed.entity()) {
          if (outputFeedEntity.has_trip_update() &&
              outputFeedEntity.trip_update().trip().trip_id() == tripID) {
            for (const transit_realtime::TripUpdate_StopTimeUpdate& update : outputFeedEntity.trip_update().stop_time_update()) {
              if (update.stop_id() == stops[closest_segment_start + 1].id_) {
                tripUpdateExists = true;
                stopTimeUpdate = update;
                break;
              }
            }
              }
        }

        if (tripUpdateExists) {
          stopTimeUpdate.mutable_arrival()->set_time(predicted_arrival);
        } else {
          transit_realtime::FeedEntity* new_entity = outputFeed.add_entity();
          new_entity->set_id(tripID);

          transit_realtime::TripUpdate* trip_update = new_entity->mutable_trip_update();
          trip_update->mutable_trip()->set_trip_id(tripID);

          transit_realtime::TripUpdate_StopTimeUpdate* stop_time_update = trip_update->add_stop_time_update();
          stop_time_update->set_stop_id(stops[closest_segment_start + 1].id_);
          stop_time_update->mutable_arrival()->set_time(predicted_arrival);

          std::cerr << "Create trip update" << timetable.locations_.ids_.size() << std::endl;
        }

      }
    }
}