#pragma once

#include <cstdint>

#include <string>
#include <tuple>
#include <unordered_map>


// Hash-Funktor für std::tuple
template <>
struct std::hash<std::tuple<std::string, std::string, std::string>> {
  size_t operator()(const tuple<string, string, string>& t) const {
    return hash<string>{}(get<0>(t)) ^ (hash<string>{}(get<1>(t)) << 1) ^
           (hash<string>{}(get<2>(t)) << 2);
  }
};


/**
 * An Event when a vehicle of a trip arrives or ends at a stop
 */
struct stopTime {
  std::string trip_id;
  std::string stop_id;
  std::int64_t arrival_time;
  std::string date;
};


/**
 * Stores historic stopTimes which are then used for prediction
 */
class stopTimeStore {
  /**
   * Create a stopTimeStore storing the memory-mapped files
   */
public:
  std::unordered_map<std::tuple<std::string, std::string, std::string>,
                     std::int64_t> stopTimes;

  void store(std::string const& trip_id,
                    std::string const& stop_id,
                    std::int64_t arrival_time,
                    std::string date);
  int64_t getAverageArrivalTime(std::string const& trip_id, std::string const& stop_id);
};
