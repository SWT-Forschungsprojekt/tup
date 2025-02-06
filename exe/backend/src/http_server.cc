#include "tup/backend/http_server.h"

#include <utility>

#include "boost/algorithm/string.hpp"
#include "boost/asio/post.hpp"
#include "boost/json.hpp"

#include "utl/pipes.h"
#include "utl/to_vec.h"

#include "net/web_server/responses.h"
#include "net/web_server/serve_static.h"
#include "net/web_server/web_server.h"

#include "geo/latlng.h"

using namespace net;
using net::web_server;

namespace http = boost::beast::http;
namespace fs = std::filesystem;
namespace json = boost::json;

namespace tup::backend {

template <typename Body>
void set_cors_headers(http::response<Body>& res) {
  using namespace boost::beast::http;
  res.set(field::access_control_allow_origin, "*");
  res.set(field::access_control_allow_headers,
          "X-Content-Type-Options, X-Requested-With, Content-Type, Accept, "
          "Authorization");
  res.set(field::access_control_allow_methods, "GET, POST, OPTIONS");
  res.set(field::access_control_max_age, "3600");
}

web_server::string_res_t json_response(
    web_server::http_req_t const& req,
    std::string const& content,
    http::status const status = http::status::ok) {
  auto res = net::string_response(req, content, status, "application/json");
  set_cors_headers(res);
  return res;
}


json::value to_json(std::vector<geo::latlng> const& polyline) {
  auto a = json::array{};
  for (auto const& p : polyline) {
    a.emplace_back(json::array{p.lng(), p.lat()});
  }
  return a;
}

struct http_server::impl {
  impl(boost::asio::io_context& ios,
       boost::asio::io_context& thread_pool,
       std::string const& static_file_path)
      : ioc_{ios},
        thread_pool_{thread_pool},
        server_{ioc_} {

  }

  void handle_request(web_server::http_req_t const& req,
                      web_server::http_res_cb_t const& cb) {
    std::cout << "[" << req.method_string() << "] " << req.target() << '\n';
    switch (req.method()) {
      case http::verb::options: return cb(json_response(req, {}));
      case http::verb::get: {
        auto const& target = req.target();
        if (target.starts_with("/api/test")) {
          return cb(json_response(req, R"({"message": "Test"})",
                                  http::status::ok));
        } else {
          return cb(json_response(req, R"({"error": "Not found"})",
                                  http::status::not_found));
        }
      }
      case http::verb::post:
      case http::verb::head: return handle_static(req, cb);
      default:
        return cb(json_response(req,
                                R"({"error": "HTTP method not supported"})",
                                http::status::bad_request));
    }
  }

  void handle_static(web_server::http_req_t const& req,
                     web_server::http_res_cb_t const& cb) {
    if (auto res = net::serve_static_file(static_file_path_, req);
        res.has_value()) {
      cb(std::move(*res));
        } else {
          namespace http = boost::beast::http;
          cb(net::web_server::string_res_t{http::status::not_found, req.version()});
        }
  }

  void listen(std::string const& host, std::string const& port) {
    server_.on_http_request(
        [this](web_server::http_req_t const& req,
               web_server::http_res_cb_t const& cb,
               bool /*ssl*/) { return handle_request(req, cb); });

    boost::system::error_code ec;
    server_.init(host, port, ec);
    if (ec) {
      std::cerr << "server init error: " << ec.message() << "\n";
      return;
    }

    std::cout << "Listening on http://" << host << ":" << port
              << "/ and https://" << host << ":" << port << "/" << '\n';
    if (host == "0.0.0.0") {
      std::cout << "Local link: http://127.0.0.1:" << port << "/" << '\n';
    }
    server_.run();
  }

  void stop() { server_.stop(); }

private:
  boost::asio::io_context& ioc_;
  boost::asio::io_context& thread_pool_;
  web_server server_;
  bool serve_static_files_{false};
  std::string static_file_path_;
};

http_server::http_server(boost::asio::io_context& ioc,
                         boost::asio::io_context& thread_pool,
                         std::string const& static_file_path)
    : impl_(new impl(ioc, thread_pool, static_file_path)) {}

http_server::~http_server() = default;

void http_server::listen(std::string const& host, std::string const& port) {
  impl_->listen(host, port);
}

void http_server::stop() { impl_->stop(); }

}  // namespace osr::backend
