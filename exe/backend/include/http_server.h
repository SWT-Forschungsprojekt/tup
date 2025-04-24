#pragma once

#include <memory>
#include <string>

#ifdef NO_DATA
#undef NO_DATA
#endif
#include "gtfs-rt/gtfs-realtime.pb.h"

#include "boost/asio/io_context.hpp"

namespace tup::backend {

  struct http_server {
    http_server(boost::asio::io_context& ioc,
                boost::asio::io_context& thread_pool,
                std::string const& static_file_path, 
                transit_realtime::FeedMessage& feed);
    ~http_server();
    http_server(http_server const&) = delete;
    http_server& operator=(http_server const&) = delete;
    http_server(http_server&&) = delete;
    http_server& operator=(http_server&&) = delete;

    void listen(std::string const& host, std::string const& port);
    void stop();

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
  };

}  // namespace tup::backend
