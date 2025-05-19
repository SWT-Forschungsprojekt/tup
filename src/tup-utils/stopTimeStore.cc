#include "tup-utils/stopTimeStore.h"

/**
   * Store the given StopTimeEvent in the hash map
   * @param trip_id to be stored
   * @param stop_id to be stored
   * @param arrival_time to be stored
   * @param date to be stored
   */
void stopTimeStore::store(std::string const& trip_id,
                          std::string const& stop_id,
                          std::int64_t arrival_time,
                          std::string date) {
  const auto key = std::make_tuple(trip_id, stop_id, date);
  stopTimes[key] = arrival_time;
}

/**
   * Retrieves the average arrival time for all similar past StopTimeEvents for Prediction
   * @param trip_id to identify similar events
   * @param stop_id to identify similar events
   * @return Average arrival time
   */
int64_t stopTimeStore::getAverageArrivalTime(std::string const& trip_id, std::string const& stop_id) {
  int64_t sum = 0;
  int64_t count = 0;
  for (auto const& stoptime : stopTimes) {
    if (std::get<0>(stoptime.first) == trip_id && std::get<1>(stoptime.first) == stop_id) {
      sum += stoptime.second;
      count++;
    }
  }
  if (count == 0) {
    return 0;
  }
  return sum / count;
}