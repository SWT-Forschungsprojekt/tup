#include "feed_updater.h"
#include <iostream>
#include <chrono>
#include <thread>

#include "net/http/client/https_client.h"

using namespace net::http::client;

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
    std::this_thread::sleep_for(std::chrono::seconds(30));
  }
}

/**
 * Downloads the vehicle Positions feed and updates the tripUpdates feed with
 * the predictionMethod
 *
 * @return whether everything went right
 */
bool FeedUpdater::downloadFeed() {
  std::string protobuf_data;
  // Boost Asio IO Service object
  // Represents an 'event loop' for asynchronous Input/Output operations
  // (such as networking or timers)
  boost::asio::io_service ios;
  request request{url_,request::method::GET};

  make_https(ios, request.address)
      ->query(request, [&protobuf_data](std::shared_ptr<net::ssl> const&, response const& res,
                          const boost::system::error_code& ec) {
        if (ec) {
          std::cout << "error: " << ec.message() << "\n";
        } else {
            protobuf_data = res.body;
        }
      });

  // Start an asynchronous event loop.
  // This is required to start the request!
  ios.run();

  transit_realtime::FeedMessage new_feed;
  if (!new_feed.ParseFromString(protobuf_data)) return false;
  // TODO: Make predictionMethod return updated feed and update feed afterwards to have atomic updates
  predictionMethod_(new_feed);
  return true;
}
