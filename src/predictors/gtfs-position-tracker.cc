#include "predictors/gtfs-position-tracker.h"
#include <nigiri/timetable.h>

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
          if (distance < 100){}
            if (tripUpdate_exists)
              updateTripupdate();
            else:
              createTripUpdate();
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