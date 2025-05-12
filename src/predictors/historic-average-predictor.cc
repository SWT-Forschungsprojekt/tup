#include "predictors/historic-average-predictor.h"
#include "predictors/predictor-utils.h"


HistoricAveragePredictor::HistoricAveragePredictor() = default;

/**
 * Predictor based on the HistoricAveragePredictor approach
 * This checks when the trip reached the following trip in the last n trips
 * and uses this as a prediction.
 *
 * @param tripUpdates current tripUpdate feed to be updated
 * @param vehiclePositions positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void HistoricAveragePredictor::predict(
    transit_realtime::FeedMessage& tripUpdates,
    const transit_realtime::FeedMessage& vehiclePositions,
    const nigiri::timetable& timetable) {
  // TODO: Implement HistoricAveragePredictor
  // Part 1: Storing of departures based on GTFS-Position-Tracker
  // Part 2: Prediction based on the stored departures/arrivals
  // Part 3: Helping method to load historic data from protobuf files
}
