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
#include "simple-ldapd/security/tls.hpp"
#include <atomic>
#include <memory>
#include <thread>

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

  const LdapConfig &config() const { return config_; }
  SchemaRegistry &schema() { return schema_; }
  Backend *backend() { return backend_.get(); }

private:
  void acceptLoop();

  LdapConfig config_;
  SchemaRegistry schema_;
  TlsContext tls_;
  SaslAuthenticator sasl_;
  Listener listener_;
  std::unique_ptr<Backend> backend_;
  std::atomic<bool> running_{false};
  std::atomic<bool> initialized_{false};
  std::thread accept_thread_;
};

}  // namespace simple_ldapd
