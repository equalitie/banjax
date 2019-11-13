#pragma once

class Socket final {
public:
  Socket()
    : _ctx(1 /* thread count */)
    , _socket(_ctx, ZMQ_PUB)
  {}

  Socket(const Socket&) = delete;

  zmq::socket_t& handle() {
    return _socket;
  }

  bool bind(std::string endpoint) {
    if (endpoint == _local_endpoint) return true;

    if (endpoint.empty()) {
      unbind();
      return true;
    }

    if (is_bound()) {
      unbind();
    }

    try {
      _socket.bind(endpoint.c_str());
      _local_endpoint = std::move(endpoint);
    } catch(const zmq::error_t&) {
      _local_endpoint = {};
      return false;
    }

    return true;
  }

  void unbind() {
    if (!is_bound()) return;
    _socket.unbind(_local_endpoint.c_str());
  }

  std::string local_endpoint() const {
    return _local_endpoint;
  }

  bool is_bound() const {
    return !_local_endpoint.empty();
  }

private:
  zmq::context_t _ctx;
  zmq::socket_t _socket;
  std::string _local_endpoint;
};

