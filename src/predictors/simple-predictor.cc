#include "predictors/simple-predictor.h"
#include <random>


SimplePredictor::SimplePredictor(std::chrono::milliseconds delay, bool random) {
  if (random && delay.count() != 0) {
    throw std::invalid_argument("Delay must be zero when random is true");
  }

  delay_ = delay;
  random_ = random;
};

int64_t SimplePredictor::getDelay() {
  if (random_) {
    static std::mt19937_64 rng(std::random_device{}());  // 64-bit engine
    std::uniform_int_distribution<int64_t> dist(0, 300000);  // 0 to 300,000 milliseconds (5 minutes)
    return dist(rng);
  } else {
    // Fixed delay logic
    return delay_.count();
  }  
};

void SimplePredictor::predict(transit_realtime::FeedMessage& message) {
  const int64_t delay = getDelay();

  for (int i = 0; i < message.entity_size(); ++i) {
    transit_realtime::FeedEntity* entity = message.mutable_entity(i);
    if (entity->has_trip_update()) {
      transit_realtime::TripUpdate* tripUpdate = entity->mutable_trip_update();
      for (int j = 0; j < tripUpdate->stop_time_update_size(); ++j) {
        transit_realtime::TripUpdate_StopTimeUpdate* stopTimeUpdate = tripUpdate->mutable_stop_time_update(j);

        if (stopTimeUpdate->has_arrival() && stopTimeUpdate->arrival().has_time()) {
          transit_realtime::TripUpdate_StopTimeEvent* arrival = stopTimeUpdate->mutable_arrival();
          arrival->set_time(arrival->time() + delay);
        }

        if (stopTimeUpdate->has_departure() && stopTimeUpdate->departure().has_time()) {
          transit_realtime::TripUpdate_StopTimeEvent* departure = stopTimeUpdate->mutable_departure();

          // Check if the arrival + delay is greater than the departure time
          if (stopTimeUpdate->has_arrival() && stopTimeUpdate->arrival().has_time()) {
            transit_realtime::TripUpdate_StopTimeEvent* arrival = stopTimeUpdate->mutable_arrival();
            if (arrival->time() + static_cast<int64_t>(delay) > departure->time()) {
              // If so, set the departure time to be the same as the arrival time
              departure->set_time(arrival->time() + delay);
            }
          }
          else {
            // If there is no arrival time, just add the delay to the departure time
            departure->set_time(departure->time() + delay);
          }
        }
      }
    }
  }
};