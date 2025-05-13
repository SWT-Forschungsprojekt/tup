#pragma once
#include <nigiri/timetable.h>
#include "tup-utils/stopTimeStore.h"
#include "gtfs-rt/gtfs-realtime.pb.h"

/**
 * Class implementing the predictor based on Historic Average Predictor from
 * transitime
 */
class HistoricAveragePredictor {
public:
  explicit HistoricAveragePredictor(const std::filesystem::path& path)
    : store_(path, cista::mmap::protection::WRITE) {}

  /**
   * Method to predict the next stop and arrival time
   * @param tripUpdates current state of the generated tripUpdates feed
   * @param vehiclePositions current VehiclePositions feed
   * @param timetable matching the realtime feeds
   */
  void predict(transit_realtime::FeedMessage& tripUpdates, 
              const transit_realtime::FeedMessage& vehiclePositions,
              const nigiri::timetable& timetable);
  void loadHistoricData(const std::vector<stopTime>* stopTimes);
  stopTimeStore store_;
};