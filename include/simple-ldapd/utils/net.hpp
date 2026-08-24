/**
 * @file net.hpp
 * @brief TCP listen helper stubs
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/utils/platform.hpp"
#include <string>

namespace simple_ldapd {

class TcpListener {
public:
  TcpListener() = default;
  ~TcpListener();

  TcpListener(const TcpListener &) = delete;
  TcpListener &operator=(const TcpListener &) = delete;

  bool bindAndListen(const std::string &address, port_t port);
  void close();
  bool isOpen() const;
  socket_t native() const;

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
};

bool initializeSockets();
void shutdownSockets();

}  // namespace simple_ldapd
