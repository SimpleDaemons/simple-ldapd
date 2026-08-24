/**
 * @file daemon.hpp
 * @brief Directory daemon
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/auth/sasl.hpp"
#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/listener.hpp"
#include "simple-ldapd/schema/registry.hpp"
#include "simple-ldapd/security/rate_limiter.hpp"
#include "simple-ldapd/security/tls.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace simple_ldapd {

class LdapDaemon {
public:
  explicit LdapDaemon(LdapConfig config);
  ~LdapDaemon();

  LdapDaemon(const LdapDaemon &) = delete;
  LdapDaemon &operator=(const LdapDaemon &) = delete;

  bool initialize();
  bool start();
  void stop();
  bool running() const;
  bool testConfig() const;
  port_t boundPort() const;
  port_t boundLdapsPort() const;

  const LdapConfig &config() const { return config_; }
  SchemaRegistry &schema() { return schema_; }
  Backend *backend() { return backend_.get(); }

private:
  struct SessionWorker {
    std::thread thread;
    std::shared_ptr<std::atomic<bool>> finished;
  };

  void acceptLoop();
  void serveConnection(TcpConnection connection, bool ldaps);
  void launchSession(TcpConnection connection, bool ldaps);
  void reapWorkers();
  void joinWorkers();

  LdapConfig config_;
  SchemaRegistry schema_;
  TlsContext tls_;
  SaslAuthenticator sasl_;
  RateLimiter rate_limiter_;
  Listener listener_;
  Listener ldaps_listener_;
  std::unique_ptr<Backend> backend_;
  std::atomic<bool> running_{false};
  std::atomic<bool> initialized_{false};
  std::thread accept_thread_;
  std::mutex workers_mutex_;
  std::vector<SessionWorker> workers_;
};

}  // namespace simple_ldapd
