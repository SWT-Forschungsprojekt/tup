#include "tup-utils/stopTimeStore.h"

void stopTimeStore::store(std::string const& trip_id,
                          std::string const& stop_id,
                          std::int64_t arrival_time,
                          std::string date) {
  const auto key = std::make_tuple(trip_id, stop_id, date);
  stopTimes[key] = arrival_time;
}

int64_t stopTimeStore::getAverageArrivalTime(std::string const& trip_id, std::string const& stop_id) {
  int64_t sum = 0;
  int64_t count = 0;
  for (auto const& stoptime : stopTimes) {
    if (std::get<0>(stoptime.first) == trip_id && std::get<1>(stoptime.first) == stop_id) {
      sum += stoptime.second;
      count++;
    }
  }
  return sum / count;
}