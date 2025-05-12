#include "predictors/schedule-based-predictor.h"
#include <nigiri/timetable.h>

#include <boost/chrono/time_point.hpp>

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
 * @param tripUpdates current tripUpdate feed to be updated
 * @param vehiclePositions positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void ScheduleBasedPredictor::predict(
    transit_realtime::FeedMessage& tripUpdates,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
    
    for (const transit_realtime::FeedEntity& entity : vehiclePositions.entity()) {
        if (!entity.has_vehicle()) {
            continue;
        }

        const  transit_realtime::VehiclePosition& vehicle_position = entity.vehicle();
        if (!vehicle_position.has_trip() || !vehicle_position.has_position()) {
            continue;
        }

        // vehicle position as Boost Geometry Punkt
        Point vehicle_point;
        boost::geometry::set<0>(vehicle_point, vehicle_position.position().longitude());
        boost::geometry::set<1>(vehicle_point, vehicle_position.position().latitude());

        std::string tripID = vehicle_position.trip().trip_id();
        std::string routeID = vehicle_position.trip().route_id();
        std::string vehicleID = vehicle_position.vehicle().id();
        const std::vector<nigiri::location>& stops = predictorUtils::get_stops_for_trip(timetable, tripID);
        
        if (stops.size() < 2) {
            continue;
        }

        // find the closest segment to the vehicle position
        size_t closest_segment_start = 0;
        double min_segment_distance = std::numeric_limits<double>::max();
        Point closest_foot_point;

        for (size_t i = 0; i < stops.size() - 1; ++i) {
            const nigiri::location& current_stop = stops[i];
            const nigiri::location& next_stop = stops[i + 1];

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
      std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
      const long current_time =
          std::chrono::duration_cast<std::chrono::seconds>(
              now.time_since_epoch())
              .count();
      const  nigiri::trip_idx_t trip_idx = predictorUtils::convert_trip_id_to_idx(timetable, tripID);
      const nigiri::paged_vecvec<cista::strong<unsigned, nigiri::_trip_idx>, cista::pair<cista::strong<unsigned, nigiri::_transport_idx>, nigiri::interval<unsigned short>>>& transport_ranges = timetable.trip_transport_ranges_;
      const cista::strong<unsigned, nigiri::_transport_idx> t_idx = transport_ranges[trip_idx][0].first; // Beachten Sie .from statt .from_
      // convert to sys_days
      const  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int, std::ratio<86400>>> today = date::floor<date::days>(now);
      const nigiri::day_idx_t current_day_idx = timetable.day_idx(today);
      nigiri::unixtime_t departure_time = timetable.event_time(
          nigiri::transport{
              t_idx,
              current_day_idx
          },
          // Convert from `location_idx_t` to `stop_idx_t`
          nigiri::stop_idx_t{static_cast<unsigned short int>(
              static_cast<unsigned>(closest_segment_start))},
          nigiri::event_type::kDep
      );
      nigiri::unixtime_t arrival_time = timetable.event_time(
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
        const long predicted_arrival = current_time + static_cast<int>(progress_time * (1 / progress_way));
        // Update the feed accordingly
        transit_realtime::TripUpdate_StopTimeUpdate stopTimeUpdate;
        bool tripUpdateExists = false;
        transit_realtime::TripUpdate* tripUpdateToUpdate;
        bool stopTimeUpdateExists = false;
        transit_realtime::TripUpdate_StopTimeEvent* arrivalToUpdate;

        for (int i = 0; i < tripUpdates.entity_size(); ++i) {
          const transit_realtime::FeedEntity& outputFeedEntity = tripUpdates.entity(i);
          if (outputFeedEntity.has_trip_update() && outputFeedEntity.trip_update().trip().trip_id() == tripID) {
            tripUpdateExists = true;
            tripUpdateToUpdate = tripUpdates.mutable_entity(i)->mutable_trip_update();
            for (int j = 0; j < outputFeedEntity.trip_update().stop_time_update_size(); ++j) {
              const transit_realtime::TripUpdate_StopTimeUpdate& update = outputFeedEntity.trip_update().stop_time_update(j);
              if (update.stop_id() == stops[closest_segment_start + 1].id_) {
                stopTimeUpdateExists = true;
                arrivalToUpdate = tripUpdateToUpdate->mutable_stop_time_update(j)->mutable_arrival();
                break;
              }
            }
          }
        }

        if (!tripUpdateExists){
          transit_realtime::FeedEntity* new_entity = tripUpdates.add_entity();
          new_entity->set_id(tripID);

          tripUpdateToUpdate = new_entity->mutable_trip_update();
          transit_realtime::TripDescriptor* trip = tripUpdateToUpdate->mutable_trip();
          trip->set_trip_id(tripID);
          trip->set_route_id(routeID);
          tripUpdateToUpdate->mutable_vehicle()->set_id(vehicleID);
        }

        if (!stopTimeUpdateExists){
          transit_realtime::TripUpdate_StopTimeUpdate* stop_time_update = tripUpdateToUpdate->add_stop_time_update();
          stop_time_update->set_stop_id(stops[closest_segment_start + 1].id_);
          arrivalToUpdate = stop_time_update->mutable_departure();
        }

        tripUpdateToUpdate->set_timestamp(current_time);
        arrivalToUpdate->set_time(predicted_arrival);

      }
    }

    transit_realtime::FeedHeader* header = tripUpdates.mutable_header();
    header->set_timestamp(time(nullptr));
}