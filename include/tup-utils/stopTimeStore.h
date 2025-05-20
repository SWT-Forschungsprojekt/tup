#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>

/**
 * Hash operation for the std::tuple type, necessary to store it in a hash map
 */
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
  std::string date;
  std::int64_t arrival_time;
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
  /**
   * Store the given StopTimeEvent in the hash map
   * @param trip_id to be stored
   * @param stop_id to be stored
   * @param arrival_time to be stored
   * @param date to be stored
   */
  void store(std::string const& trip_id,
                    std::string const& stop_id,
                    std::int64_t arrival_time,
                    std::string date);
  /**
   * Retrieves the average arrival time for all similar past StopTimeEvents for Prediction
   * @param trip_id to identify similar events
   * @param stop_id to identify similar events
   * @return Average arrival time
   */
  int64_t getAverageArrivalTime(std::string const& trip_id, std::string const& stop_id);
};
