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

}  // namespace simple_ldapd
