/**
 * @file net.cpp
 * @brief TCP listen helper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/utils/net.hpp"

#include "simple-ldapd/utils/logger.hpp"

namespace simple_ldapd {

bool initializeSockets() {
#ifdef SIMPLE_LDAPD_WINDOWS
  WSADATA data;
  return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
  return true;
#endif
}

void shutdownSockets() {
#ifdef SIMPLE_LDAPD_WINDOWS
  WSACleanup();
#endif
}

TcpListener::~TcpListener() { close(); }

bool TcpListener::bindAndListen(const std::string &address, port_t port) {
  close();
  if (!initializeSockets()) {
    return false;
  }
  fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ == INVALID_SOCKET_VALUE) {
    Logger::instance().error("socket() failed");
    return false;
  }
  int reuse = 1;
#ifdef SIMPLE_LDAPD_WINDOWS
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse),
             sizeof(reuse));
#else
  setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (address.empty() || address == "0.0.0.0") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    if (::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
      Logger::instance().error("invalid listen address: " + address);
      close();
      return false;
    }
  }
  if (::bind(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
    Logger::instance().warning("bind() failed; listener not active");
    close();
    return false;
  }
  if (::listen(fd_, 16) != 0) {
    Logger::instance().warning("listen() failed");
    close();
    return false;
  }
  return true;
}

void TcpListener::close() {
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
}

bool TcpListener::isOpen() const { return fd_ != INVALID_SOCKET_VALUE; }

socket_t TcpListener::native() const { return fd_; }

}  // namespace simple_ldapd
