/**
 * @file listener.cpp
 * @brief TCP listener wrapper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/core/listener.hpp"

namespace simple_ldapd {

bool Listener::start(const std::string &address, port_t port) {
  return tcp_.bindAndListen(address, port);
}

void Listener::stop() { tcp_.close(); }

bool Listener::running() const { return tcp_.isOpen(); }

port_t Listener::boundPort() const { return tcp_.boundPort(); }

socket_t Listener::native() const { return tcp_.native(); }

std::optional<TcpConnection> Listener::acceptConnection(int timeout_ms) {
  return tcp_.acceptConnection(timeout_ms);
}

}  // namespace simple_ldapd
