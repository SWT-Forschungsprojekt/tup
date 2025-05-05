#include "predictors/schedule-based-predictor.h"
#include <nigiri/timetable.h>


ScheduleBasedPredictor::ScheduleBasedPredictor() = default;

/**
 * Predictor based on a given schedule
 * This checks where the vehicle currently is in the schedule and checks if it
 * is too late. If so, it interpolates how much time the vehicle will be too
 * late at the next stop
 * @param outputFeed current tripUpdate feed to be updated
 * @param vehiclePositionFeed positions of vehicles as feed
 * @param timetable timetable to match the vehiclePositions to stops
 */
void ScheduleBasedPredictor::predict(
    transit_realtime::FeedMessage& outputFeed,
    const transit_realtime::FeedMessage& vehiclePositionFeed,
    const nigiri::timetable& timetable) {

}
