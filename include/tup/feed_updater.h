#ifndef FEED_UPDATER_H
#define FEED_UPDATER_H

#include <atomic>
#include <string>
#include <functional>
#include <thread>
#ifdef NO_DATA
#undef NO_DATA
#endif
#include "gtfs-rt/gtfs-realtime.pb.h"

#include <functional>

class FeedUpdater {
public:
  using PredictionMethod = std::function<transit_realtime::FeedMessage(const transit_realtime::FeedMessage&)>;

  FeedUpdater(transit_realtime::FeedMessage& feed, const std::string& url, PredictionMethod predictionMethod)
      : feed_(feed), url_(url), predictionMethod_(std::move(predictionMethod)) {}
  ~FeedUpdater();
  void start();
  void stop();
  transit_realtime::FeedMessage& getFeed();

private:
  void run();
  bool downloadFeed();

  transit_realtime::FeedMessage& feed_;
  std::string url_;
  std::thread worker_;
  std::atomic<bool> running_{true};
  PredictionMethod predictionMethod_;
};


#endif  // FEED_UPDATER_H