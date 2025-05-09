#include "predictors/predictor-utils.h"
#include <nigiri/timetable.h>

/**
 * Converts a trip id to the index where the trip is stored in memory
 * @param timetable to look up the location
 * @param trip_id to convert
 * @return index of the trip
 */
auto predictorUtils::convert_trip_id_to_idx(nigiri::timetable const& timetable, std::string const& trip_id) -> nigiri::trip_idx_t {
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

/**
 * Returns all stops that belong to a route of a trip
 * @param timetable timetable to look up the stops belonging to a trip
 * @param trip_id of the trip the stops belong to
 * @return all stops that belong to a route of a trip
 */
std::vector<nigiri::location> predictorUtils::get_stops_for_trip(nigiri::timetable const& timetable,
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
