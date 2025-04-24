#pragma once

#include <chrono>
#include "gtfs-rt/gtfs-realtime.pb.h"

class SimplePredictor {
    public:
        SimplePredictor(std::chrono::milliseconds delay, bool random);
        
        // Method to predict the next stop and arrival time
        void predict(transit_realtime::FeedMessage& message);
        std::chrono::milliseconds delay_;

    private:
        int64_t getDelay();
        bool random_;
};