#include "feed_updater.h"
#include <iostream>
#include <curl/curl.h>
#include <chrono>
#include <thread>

FeedUpdater::~FeedUpdater() {
  stop();
}

void FeedUpdater::start() {
  worker_ = std::thread(&FeedUpdater::run, this);
}

void FeedUpdater::stop() {
  running_ = false;
  if (worker_.joinable()) {
    worker_.join();
  }
}

transit_realtime::FeedMessage& FeedUpdater::getFeed() {
  return feed_;
}

void FeedUpdater::run() {
  while (running_) {
    if (!downloadFeed()) {
      std::cerr << "Fehler beim Aktualisieren des Feeds." << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));  // Alle 10 Sekunden aktualisieren
  }
}

bool FeedUpdater::downloadFeed() {
  std::string protobuf_data;
  CURL* curl = curl_easy_init();
  if (!curl) return false;

  curl_easy_setopt(curl, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &protobuf_data);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) return false;

  transit_realtime::FeedMessage new_feed;
  if (!new_feed.ParseFromString(protobuf_data)) return false;

  feed_ = predictionMethod_(new_feed);  // Aktualisiere den Feed
  return true;
}
