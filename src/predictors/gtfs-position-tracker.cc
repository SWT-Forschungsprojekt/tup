#include "predictors/gtfs-position-tracker.h"
#include <nigiri/timetable.h>
#include <chrono>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

GTFSPositionTracker::GTFSPositionTracker() = default;

std::vector<nigiri::location> get_stops_for_trip(nigiri::timetable timetable, const std::string& string);
void GTFSPositionTracker::predict(
    transit_realtime::FeedMessage& message,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
    for (int i = 0; i < vehiclePositions.entity_size(); i++) {
      transit_realtime::FeedEntity* entity = message.mutable_entity(i);
      if (entity->has_vehicle()) {
        transit_realtime::VehiclePosition* vehicle_position = entity->mutable_vehicle();
        // Get Trip ID
        std::string tripID = vehicle_position->trip().trip_id();
        // Get a stop list for a given trip
        std::vector<nigiri::location> stop_list = get_stops_for_trip(timetable, tripID);
        // check for each stop if we are close
        for (nigiri::location location : stop_list) {

          namespace bg = boost::geometry;
          bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>
              vehicle_point;
          bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>
              location_point;

          bg::set<0>(vehicle_point, vehicle_position->position().longitude());
          bg::set<1>(vehicle_point, vehicle_position->position().latitude());
          bg::set<0>(location_point, location.pos_.lng_());
          bg::set<1>(location_point, location.pos_.lat_());
          const auto distance = bg::distance(
              vehicle_point, location_point,
              bg::strategy::distance::haversine(6371000.0));
          if (distance < 100) {
            transit_realtime::TripUpdate_StopTimeUpdate stopTimeUpdate;
            bool tripUpdateExists = false;
            for (const auto& tripUpdate : message.entity()) {
              if (tripUpdate.has_trip_update() &&
                  tripUpdate.trip_update().trip().trip_id() == tripID) {
                for (const auto& update :
                     tripUpdate.trip_update().stop_time_update()) {
                  if (update.stop_id() == location.id_) {
                    tripUpdateExists = true;
                    stopTimeUpdate = update;
                    break;
                  }
                }
              }
            }
            auto now = std::chrono::system_clock::now();
            auto current_time =
                  std::chrono::duration_cast<std::chrono::seconds>(
                      now.time_since_epoch())
                      .count();
            if (tripUpdateExists) {
              stopTimeUpdate.mutable_departure()->set_time(
                  std::max(current_time, stopTimeUpdate.departure().time()));
            }
            else {
              transit_realtime::FeedEntity* new_entity = message.add_entity();
              new_entity->set_id(tripID);

              transit_realtime::TripUpdate* trip_update =
                  new_entity->mutable_trip_update();
              trip_update->mutable_trip()->set_trip_id(tripID);

              transit_realtime::TripUpdate_StopTimeUpdate* stop_time_update =
                  trip_update->add_stop_time_update();
              stop_time_update->set_stop_id(location.id_);
              stop_time_update->mutable_departure()->set_time(current_time);
            }

          }
        }
      }
    }};

std::vector<nigiri::location> get_stops_for_trip(nigiri::timetable const& tt, 
                                                std::string const& trip_id_str) {
    // Trip-ID aus String erstellen
    auto const trip_id = nigiri::trip_id{trip_id_str};
    
    // Trip-ID in internen Index umwandeln
    auto const trip_id_pair = std::ranges::find_if(tt.trip_id_to_idx_,
        [&](auto const& pair) {
            return tt.trip_id_strings_[pair.first] == trip_id;
        });
    
    if (trip_id_pair == tt.trip_id_to_idx_.end()) {
        throw std::runtime_error("Trip ID nicht gefunden: " + trip_id_str);
    }
    
    auto const trip_idx = trip_id_pair->second;
    
    // Route für den Trip finden
    auto const& transports = tt.trip_transport_ranges_[trip_idx];
    if (transports.empty()) {
        throw std::runtime_error("Keine Transporte für diesen Trip gefunden");
    }
    
    // Ersten Transport nehmen und dessen Route
    auto const first_transport = transports[0];
    auto const route_idx = tt.transport_route_.at(first_transport.first); // Änderung hier: .first für transport_idx
    
    // Stops aus der Route-Sequenz holen
    std::vector<nigiri::location> stops;
    auto const& stop_sequence = tt.route_location_seq_[route_idx];
    
    // Jeden Stop in ein location-Objekt umwandeln
    for (auto const stop_idx : stop_sequence) {
        stops.push_back(tt.locations_.get(nigiri::location_idx_t(stop_idx)));
    }
    
    return stops;
}