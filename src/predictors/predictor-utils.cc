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
    nigiri::trip_idx_t trip_idx;
    try {
        trip_idx = convert_trip_id_to_idx(timetable, trip_id);
    } catch (const std::runtime_error& e) {
      std::cerr << "Fehler: " << e.what() << std::endl;
      return {};
    }

    if (trip_idx >= timetable.trip_transport_ranges_.size()) {
        throw std::runtime_error("Trip-Index außerhalb des gültigen Bereichs");
    }

    auto const& transports = timetable.trip_transport_ranges_[trip_idx];

    if (transports.empty()) {
      throw std::runtime_error("Keine Transporte für Trip-ID: " + trip_id);
    }

    // Ersten Transport nehmen und dessen Route
    const cista::strong<unsigned, nigiri::_transport_idx> first = transports[0].first;
    nigiri::route_idx_t const route_idx = timetable.transport_route_.at(first);

    // Stops aus der Route-Sequenz holen
    std::vector<nigiri::location> stops;
    auto const& stop_sequence = timetable.route_location_seq_[route_idx];

    // Jeden Stop in ein location-Objekt umwandeln
    for (unsigned const location_idx_t : stop_sequence) {
      nigiri::location_idx_t const stop_idx = nigiri::stop{location_idx_t}.location_idx();
      stops.push_back(timetable.locations_.get(nigiri::location_idx_t(stop_idx)));
    }
    return stops;
}


/**
 * Deletes all trip updates that are not in the current trip IDs
 * @param currentTripIDs to check if the trip update is still valid
 * @param outputFeed to delete the trip updates from
 */
void predictorUtils::delete_old_trip_updates(const std::unordered_set<std::string> currentTripIDs, transit_realtime::FeedMessage& outputFeed) {
  // Remove TripUpdates for trips not in vehiclePositions anymore
  for (int i = outputFeed.entity_size() - 1; i >= 0; --i) {
    const transit_realtime::FeedEntity& outputFeedEntity = outputFeed.entity(i);
    if (outputFeedEntity.has_trip_update()) {
      const transit_realtime::TripUpdate& tripUpdate = outputFeedEntity.trip_update();
      if (currentTripIDs.contains(tripUpdate.trip().trip_id())) {
        outputFeed.mutable_entity()->DeleteSubrange(i, 1);
      }
    }
  }
}

/**
 * Sets the stop update for a given trip id and stop id. If the trip update does not exist, it will be created. If the stop update does not exist, it will be created.
 * @param tripID of the trip to set the update for
 * @param stopID of the stop to set the update for
 * @param vehicleID of the vehicle to set the update for
 * @param routeID of the route to set the update for
 * @param newArivalTime of the stop to set the update for
 * @param outputFeed to set the trip update in
 */
void predictorUtils::set_trip_update(std::string tripID, std::string_view stopID, std::string vehicleID, std::string routeID, int64_t newArivalTime, transit_realtime::FeedMessage& outputFeed){
  bool tripUpdateExists = false;
  transit_realtime::TripUpdate* tripUpdateToUpdate;
  bool stopTimeUpdateExists = false;
  transit_realtime::TripUpdate_StopTimeEvent* ariavalToUpdate;

  for (int i = 0; i < outputFeed.entity_size(); ++i) {
    const transit_realtime::FeedEntity& outputFeedEntity = outputFeed.entity(i);
    if (outputFeedEntity.has_trip_update() && outputFeedEntity.trip_update().trip().trip_id() == tripID) {
      tripUpdateExists = true;
      tripUpdateToUpdate = outputFeed.mutable_entity(i)->mutable_trip_update();
        for (int j = 0; j < outputFeedEntity.trip_update().stop_time_update_size(); ++j) {
        const transit_realtime::TripUpdate_StopTimeUpdate& update = outputFeedEntity.trip_update().stop_time_update(j);
        if (update.stop_id() == stopID) {
          stopTimeUpdateExists = true;
          ariavalToUpdate = tripUpdateToUpdate->mutable_stop_time_update(j)->mutable_arrival();
          break;
        }
      }
    }
  }
  
  if (!tripUpdateExists){
    transit_realtime::FeedEntity* new_entity = outputFeed.add_entity();
    new_entity->set_id(tripID);

    tripUpdateToUpdate = new_entity->mutable_trip_update();
    transit_realtime::TripDescriptor* trip = tripUpdateToUpdate->mutable_trip();
    trip->set_trip_id(tripID);
    trip->set_route_id(routeID);
    tripUpdateToUpdate->mutable_vehicle()->set_id(vehicleID);
  }

  if (!stopTimeUpdateExists){
    transit_realtime::TripUpdate_StopTimeUpdate* stop_time_update = tripUpdateToUpdate->add_stop_time_update();
    stop_time_update->set_stop_id(stopID);
    ariavalToUpdate = stop_time_update->mutable_arrival();
  }
  
  auto now = std::chrono::system_clock::now();
  auto current_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  
  tripUpdateToUpdate->set_timestamp(current_time);
  ariavalToUpdate->set_time(newArivalTime);
}