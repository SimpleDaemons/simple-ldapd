/**
 * @file connection.cpp
 * @brief Connection handle
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/core/connection.hpp"

namespace simple_ldapd {

Connection::Connection() = default;

Connection::Connection(socket_t fd, std::string peer)
    : fd_(fd), peer_(std::move(peer)) {}

}  // namespace simple_ldapd
