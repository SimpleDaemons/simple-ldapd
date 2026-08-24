/**
 * @file listener.hpp
 * @brief LDAP TCP listener
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/utils/net.hpp"
#include <optional>
#include <string>

namespace simple_ldapd {

class Listener {
public:
  bool start(const std::string &address, port_t port);
  void stop();
  bool running() const;
  port_t boundPort() const;
  socket_t native() const;
  std::optional<TcpConnection> acceptConnection(int timeout_ms = 200);

private:
  TcpListener tcp_;
};

}  // namespace simple_ldapd
