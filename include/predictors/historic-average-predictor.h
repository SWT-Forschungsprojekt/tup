#pragma once
#include <nigiri/timetable.h>
#include "tup-utils/stopTimeStore.h"
#include "gtfs-rt/gtfs-realtime.pb.h"

/**
 * Class implementing the predictor based on Historic Average Predictor from
 * Transitime
 */
class HistoricAveragePredictor {
public:
  HistoricAveragePredictor();
  /**
   * Method to predict the next stop and arrival time
   * @param tripUpdates current state of the generated tripUpdates feed
   * @param vehiclePositions current VehiclePositions feed
   * @param timetable matching the realtime feeds
   */
  void predict(transit_realtime::FeedMessage& tripUpdates, 
              const transit_realtime::FeedMessage& vehiclePositions,
              const nigiri::timetable& timetable);
  /**
   * load Historic Data into store to allow to load collected protobuf file
   * @param stopTimes vector of stop times that should be stored in the store
   */
  void loadHistoricData(const std::vector<stopTime>& stopTimes);
  stopTimeStore store_;
};