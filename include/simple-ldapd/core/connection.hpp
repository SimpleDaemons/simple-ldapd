/**
 * @file connection.hpp
 * @brief Client connection handle
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/utils/platform.hpp"
#include <string>

namespace simple_ldapd {

class Connection {
public:
  Connection();
  explicit Connection(socket_t fd, std::string peer);

  socket_t fd() const { return fd_; }
  const std::string &peer() const { return peer_; }
  bool tls() const { return tls_; }
  void setTls(bool enabled) { tls_ = enabled; }

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
  std::string peer_;
  bool tls_{false};
};

}  // namespace simple_ldapd
