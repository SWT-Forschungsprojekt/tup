#pragma once

#include <cstdint>

#include <string>
#include <unordered_map>

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
