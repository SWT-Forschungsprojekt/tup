#include "predictors/gtfs-position-tracker.h"
#include <nigiri/timetable.h>
#include <chrono>

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/strategies/spherical/distance_haversine.hpp>

GTFSPositionTracker::GTFSPositionTracker() = default;

// Function declarations
std::vector<nigiri::location> get_stops_for_trip(nigiri::timetable const& timetable, std::string const& trip_id);
auto convert_trip_id_to_idx(nigiri::timetable const& timetable, std::string const& trip_id) -> nigiri::trip_idx_t;

/**
 * Predictor based on the GTFS-Position-tracker approach
 * This checks if any of the vehicle Positions is close to a stop. If so,
 * a tripUpdate for the according trip and stop will be created.
 *
 * @param outputFeed current tripUpdate feed to be updated
 * @param vehiclePositionFeed positions of vehicles as feed
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
        std::vector<nigiri::location> stop_list = get_stops_for_trip(timetable, tripID);
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

/**
 * Returns all stops that belong to a route of a trip
 * @param timetable timetable to look up the stops belonging to a trip
 * @param trip_id of the trip the stops belong to
 * @return all stops that belong to a route of a trip
 */
std::vector<nigiri::location> get_stops_for_trip(nigiri::timetable const& timetable,
                                                std::string const& trip_id) {
    std::cerr << "Suche Trip-ID: " << trip_id << std::endl;
    
    nigiri::trip_idx_t trip_idx;
    try {
        trip_idx = convert_trip_id_to_idx(timetable, trip_id);
    } catch (const std::runtime_error& e) {
        std::cerr << "Fehler: " << e.what() << std::endl;
        return {};
    }
    
    std::cerr << "Gefundener Trip-Index: " << trip_idx << std::endl;
    std::cerr << "Anzahl der Transporte: " << timetable.trip_transport_ranges_.size() << std::endl;
    
    if (trip_idx >= timetable.trip_transport_ranges_.size()) {
        throw std::runtime_error("Trip-Index außerhalb des gültigen Bereichs");
    }
    
    auto const& transports = timetable.trip_transport_ranges_[trip_idx];
    std::cerr << "Anzahl der Transporte für diesen Trip: " << transports.size() << std::endl;
    
    if (transports.empty()) {
      throw std::runtime_error("Keine Transporte für Trip-ID: " + trip_id);
    }

    // Nur Debug-Meldung vor dem Zugriff
    std::cerr << "Versuche Zugriff auf ersten Transport..." << std::endl;
    // Ersten Transport nehmen und dessen Route
    const cista::strong<unsigned, nigiri::_transport_idx> first = transports[0].first;
    std::cerr << "Erster Transport erfolgreich gelesen" << std::endl;
    std::cerr << "Größe von transport_route_: " << timetable.transport_route_.size() << std::endl;
    std::cerr << "Versuche Zugriff auf Transport-Route..." << std::endl;
    nigiri::route_idx_t const route_idx = timetable.transport_route_.at(first);

    // Stops aus der Route-Sequenz holen
    std::vector<nigiri::location> stops;
    auto const& stop_sequence = timetable.route_location_seq_[route_idx];
    
    // Jeden Stop in ein location-Objekt umwandeln
    std::cerr << "Versuche Zugriff auf Transport-Route..." << std::endl;
    std::cerr << "Size of timetable.locations_.ids_" << timetable.locations_.ids_.size() << std::endl;
    for (unsigned const location_idx_t : stop_sequence) {
      nigiri::location_idx_t const stop_idx = nigiri::stop{location_idx_t}.location_idx();
      stops.push_back(timetable.locations_.get(nigiri::location_idx_t(stop_idx)));
    }
    return stops;
}

/**
 * Converts a trip id to the index where the trip is stored in memory
 * @param timetable to look up the location
 * @param trip_id to convert
 * @return index of the trip
 */
auto convert_trip_id_to_idx(nigiri::timetable const& timetable, std::string const& trip_id) -> nigiri::trip_idx_t {
    // Iteriere durch trip_id_strings_ mit korrektem Indextyp
    for (std::size_t i = 0; i < timetable.trip_id_strings_.size(); ++i) {
        if (timetable.trip_id_strings_[nigiri::trip_id_idx_t{static_cast<unsigned>(i)}].view() == trip_id) {
            // Finde das entsprechende Paar in trip_id_to_idx_
            for (auto const& pair : timetable.trip_id_to_idx_) {
                if (pair.first == nigiri::trip_id_idx_t{static_cast<unsigned>(i)}) {
                    return pair.second;
                }
            }
        }
    }
    throw std::runtime_error("Trip ID nicht gefunden: " + trip_id);
}