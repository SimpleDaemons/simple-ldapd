/**
 * @file net.cpp
 * @brief TCP listen and connection helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/utils/net.hpp"

#include "simple-ldapd/security/tls.hpp"
#include "simple-ldapd/utils/logger.hpp"

#ifdef SIMPLE_LDAPD_SSL
#include <openssl/ssl.h>
#endif

#ifndef SIMPLE_LDAPD_WINDOWS
#include <netdb.h>
#include <poll.h>
#endif

namespace simple_ldapd {

namespace {

bool recvAll(socket_t fd, uint8_t *data, size_t size) {
  size_t got = 0;
  while (got < size) {
#ifdef SIMPLE_LDAPD_WINDOWS
    const int n = ::recv(fd, reinterpret_cast<char *>(data + got),
                         static_cast<int>(size - got), 0);
#else
    const ssize_t n = ::recv(fd, data + got, size - got, 0);
#endif
    if (n <= 0) {
      return false;
    }
    got += static_cast<size_t>(n);
  }
  return true;
}

bool sendAllBytes(socket_t fd, const uint8_t *data, size_t size) {
  size_t sent = 0;
  while (sent < size) {
#ifdef SIMPLE_LDAPD_WINDOWS
    const int n = ::send(fd, reinterpret_cast<const char *>(data + sent),
                         static_cast<int>(size - sent), 0);
#else
    const ssize_t n = ::send(fd, data + sent, size - sent, 0);
#endif
    if (n <= 0) {
      return false;
    }
    sent += static_cast<size_t>(n);
  }
  return true;
}

bool pollReadable(socket_t fd, int timeout_ms) {
#ifdef SIMPLE_LDAPD_WINDOWS
  WSAPOLLFD pfd{};
  pfd.fd = fd;
  pfd.events = POLLIN;
  return WSAPoll(&pfd, 1, timeout_ms) > 0;
#else
  pollfd pfd{};
  pfd.fd = fd;
  pfd.events = POLLIN;
  return ::poll(&pfd, 1, timeout_ms) > 0;
#endif
}

bool pollWritable(socket_t fd, int timeout_ms) {
#ifdef SIMPLE_LDAPD_WINDOWS
  WSAPOLLFD pfd{};
  pfd.fd = fd;
  pfd.events = POLLOUT;
  return WSAPoll(&pfd, 1, timeout_ms) > 0;
#else
  pollfd pfd{};
  pfd.fd = fd;
  pfd.events = POLLOUT;
  return ::poll(&pfd, 1, timeout_ms) > 0;
#endif
}

#ifdef SIMPLE_LDAPD_SSL
bool sslWait(socket_t fd, SSL *ssl, int result, int timeout_ms) {
  const int err = SSL_get_error(ssl, result);
  if (err == SSL_ERROR_WANT_READ) {
    return pollReadable(fd, timeout_ms);
  }
  if (err == SSL_ERROR_WANT_WRITE) {
    return pollWritable(fd, timeout_ms);
  }
  return false;
}

bool sslHandshake(socket_t fd, SSL *ssl, bool server) {
  for (;;) {
    const int rc = server ? SSL_accept(ssl) : SSL_connect(ssl);
    if (rc == 1) {
      return true;
    }
    if (!sslWait(fd, ssl, rc, 5000)) {
      return false;
    }
  }
}

bool sslRecvAll(socket_t fd, SSL *ssl, uint8_t *data, size_t size) {
  size_t got = 0;
  while (got < size) {
    const int n = SSL_read(ssl, data + got, static_cast<int>(size - got));
    if (n > 0) {
      got += static_cast<size_t>(n);
      continue;
    }
    if (!sslWait(fd, ssl, n, 5000)) {
      return false;
    }
  }
  return true;
}

bool sslSendAll(socket_t fd, SSL *ssl, const uint8_t *data, size_t size) {
  size_t sent = 0;
  while (sent < size) {
    const int n = SSL_write(ssl, data + sent, static_cast<int>(size - sent));
    if (n > 0) {
      sent += static_cast<size_t>(n);
      continue;
    }
    if (!sslWait(fd, ssl, n, 5000)) {
      return false;
    }
  }
  return true;
}
#endif

}  // namespace

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

TcpConnection::TcpConnection(socket_t fd, std::string peer)
    : fd_(fd), peer_(std::move(peer)) {}

TcpConnection::TcpConnection(TcpConnection &&other) noexcept
    : fd_(other.fd_), peer_(std::move(other.peer_)), ssl_(other.ssl_) {
  other.fd_ = INVALID_SOCKET_VALUE;
  other.ssl_ = nullptr;
}

TcpConnection &TcpConnection::operator=(TcpConnection &&other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    peer_ = std::move(other.peer_);
    ssl_ = other.ssl_;
    other.fd_ = INVALID_SOCKET_VALUE;
    other.ssl_ = nullptr;
  }
  return *this;
}

TcpConnection::~TcpConnection() { close(); }

bool TcpConnection::valid() const { return fd_ != INVALID_SOCKET_VALUE; }

void TcpConnection::close() {
#ifdef SIMPLE_LDAPD_SSL
  if (ssl_ != nullptr) {
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    ssl_ = nullptr;
  }
#endif
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
}

bool TcpConnection::handshakeTls(const TlsContext &ctx, bool server, const std::string &sni) {
#ifdef SIMPLE_LDAPD_SSL
  if (!valid() || ssl_ != nullptr || ctx.native() == nullptr) {
    return false;
  }
  SSL *ssl = SSL_new(static_cast<SSL_CTX *>(ctx.native()));
  if (ssl == nullptr) {
    return false;
  }
  SSL_set_fd(ssl, static_cast<int>(fd_));
  if (!server && !sni.empty()) {
    SSL_set_tlsext_host_name(ssl, sni.c_str());
  }
  if (!sslHandshake(fd_, ssl, server)) {
    SSL_free(ssl);
    return false;
  }
  ssl_ = ssl;
  return true;
#else
  (void)ctx;
  (void)server;
  (void)sni;
  return false;
#endif
}

bool TcpConnection::sendAll(const std::vector<uint8_t> &data) {
  if (!valid()) {
    return false;
  }
#ifdef SIMPLE_LDAPD_SSL
  if (ssl_ != nullptr) {
    return sslSendAll(fd_, ssl_, data.data(), data.size());
  }
#endif
  return sendAllBytes(fd_, data.data(), data.size());
}

bool TcpConnection::recvExact(uint8_t *data, size_t size) {
  if (!valid()) {
    return false;
  }
#ifdef SIMPLE_LDAPD_SSL
  if (ssl_ != nullptr) {
    return sslRecvAll(fd_, ssl_, data, size);
  }
#endif
  return recvAll(fd_, data, size);
}

bool TcpConnection::waitReadable(int timeout_ms) const {
  if (!valid()) {
    return false;
  }
#ifdef SIMPLE_LDAPD_SSL
  if (ssl_ != nullptr && SSL_pending(ssl_) > 0) {
    return true;
  }
#endif
  return pollReadable(fd_, timeout_ms);
}

bool TcpConnection::recvPdu(std::vector<uint8_t> &pdu) {
  pdu.clear();
  uint8_t prefix[2];
  if (!recvExact(prefix, 2)) {
    return false;
  }
  pdu.insert(pdu.end(), prefix, prefix + 2);
  size_t length = 0;
  if ((prefix[1] & 0x80) == 0) {
    length = prefix[1];
  } else {
    const size_t count = prefix[1] & 0x7f;
    if (count == 0 || count > 4) {
      return false;
    }
    std::vector<uint8_t> raw(count);
    if (!recvExact(raw.data(), count)) {
      return false;
    }
    pdu.insert(pdu.end(), raw.begin(), raw.end());
    for (uint8_t byte : raw) {
      length = (length << 8) | byte;
    }
  }
  if (length > 1024 * 1024) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  const size_t offset = pdu.size();
  pdu.resize(offset + length);
  return recvExact(pdu.data() + offset, length);
}

std::optional<TcpConnection> TcpConnection::connectTo(const std::string &host,
                                                      port_t port) {
  if (!initializeSockets()) {
    return std::nullopt;
  }
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  const std::string port_text = std::to_string(port);
  addrinfo *result = nullptr;
  if (::getaddrinfo(host.c_str(), port_text.c_str(), &hints, &result) != 0 ||
      result == nullptr) {
    return std::nullopt;
  }
  socket_t fd = INVALID_SOCKET_VALUE;
  for (addrinfo *ai = result; ai != nullptr; ai = ai->ai_next) {
    fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd == INVALID_SOCKET_VALUE) {
      continue;
    }
    if (::connect(fd, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0) {
      break;
    }
    CLOSE_SOCKET(fd);
    fd = INVALID_SOCKET_VALUE;
  }
  ::freeaddrinfo(result);
  if (fd == INVALID_SOCKET_VALUE) {
    return std::nullopt;
  }
  return TcpConnection(fd, host + ":" + std::to_string(port));
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
  } else if (address == "127.0.0.1" || address == "localhost") {
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  } else if (::inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
    Logger::instance().error("invalid listen address: " + address);
    close();
    return false;
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

port_t TcpListener::boundPort() const {
  if (!isOpen()) {
    return 0;
  }
  sockaddr_in addr{};
#ifdef SIMPLE_LDAPD_WINDOWS
  int len = sizeof(addr);
#else
  socklen_t len = sizeof(addr);
#endif
  if (::getsockname(fd_, reinterpret_cast<sockaddr *>(&addr), &len) != 0) {
    return 0;
  }
  return ntohs(addr.sin_port);
}

std::optional<TcpConnection> TcpListener::acceptConnection(int timeout_ms) {
  if (!isOpen()) {
    return std::nullopt;
  }
  if (timeout_ms >= 0 && !pollReadable(fd_, timeout_ms)) {
    return std::nullopt;
  }
  sockaddr_in addr{};
#ifdef SIMPLE_LDAPD_WINDOWS
  int len = sizeof(addr);
#else
  socklen_t len = sizeof(addr);
#endif
  const socket_t client = ::accept(fd_, reinterpret_cast<sockaddr *>(&addr), &len);
  if (client == INVALID_SOCKET_VALUE) {
    return std::nullopt;
  }
  char host[INET_ADDRSTRLEN] = {0};
  ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof(host));
  return TcpConnection(client, std::string(host) + ":" +
                                   std::to_string(ntohs(addr.sin_port)));
}

}  // namespace simple_ldapd
