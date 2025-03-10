#pragma once
#include <atomic>
#include <string>
#include <thread>
#ifdef NO_DATA
#undef NO_DATA
#endif
#include "gtfs-rt/gtfs-realtime.pb.h"

class FeedUpdater {
public:
  FeedUpdater(transit_realtime::FeedMessage& feed, const std::string& url);
  ~FeedUpdater();
  void start();
  void stop();

private:
  void run();
  bool downloadFeed();

  transit_realtime::FeedMessage& feed_;
  std::string url_;
  std::thread worker_;
  std::atomic<bool> running_{true};
};
