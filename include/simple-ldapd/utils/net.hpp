/**
 * @file net.hpp
 * @brief TCP listen and connection helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/utils/platform.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct ssl_st;
struct ssl_ctx_st;

namespace simple_ldapd {

class TlsContext;

class TcpConnection {
public:
  TcpConnection() = default;
  explicit TcpConnection(socket_t fd, std::string peer = {});
  TcpConnection(TcpConnection &&other) noexcept;
  TcpConnection &operator=(TcpConnection &&other) noexcept;
  ~TcpConnection();

  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  bool valid() const;
  void close();
  const std::string &peer() const { return peer_; }
  socket_t native() const { return fd_; }
  bool tls() const { return ssl_ != nullptr; }

  bool handshakeTls(const TlsContext &ctx, bool server, const std::string &sni = {});
  bool tlsPeerVerified() const;
  std::string tlsPeerIdentity() const;
  bool sendAll(const std::vector<uint8_t> &data);
  bool recvExact(uint8_t *data, size_t size);
  bool recvPdu(std::vector<uint8_t> &pdu);
  bool waitReadable(int timeout_ms) const;

  static std::optional<TcpConnection> connectTo(const std::string &host, port_t port);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
  std::string peer_;
  ssl_st *ssl_{nullptr};
};

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
  port_t boundPort() const;
  std::optional<TcpConnection> acceptConnection(int timeout_ms = -1);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
};

bool initializeSockets();
void shutdownSockets();

}  // namespace simple_ldapd
