/**
 * @file daemon.cpp
 * @brief Directory daemon
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/core/daemon.hpp"

#include "simple-ldapd/backend/ldif.hpp"
#include "simple-ldapd/backend/memory.hpp"
#include "simple-ldapd/core/session.hpp"
#include "simple-ldapd/utils/logger.hpp"
#include "simple-ldapd/version.hpp"
#include <string>

namespace simple_ldapd {

LdapDaemon::LdapDaemon(LdapConfig config) : config_(std::move(config)) {}

LdapDaemon::~LdapDaemon() { stop(); }

bool LdapDaemon::initialize() {
  if (initialized_.load()) {
    return true;
  }
  if (!config_.validate()) {
    Logger::instance().error("invalid configuration");
    return false;
  }
  if (!config_.log_file.empty()) {
    Logger::instance().setLogFile(config_.log_file);
  }
  schema_.loadDirectory(config_.schema_dir);
  if (!config_.ldif_file.empty() || config_.backend == "ldif") {
    backend_ = std::make_unique<LdifBackend>(config_.ldif_file);
  } else {
    backend_ = std::make_unique<MemoryBackend>();
  }
  if (!backend_->initialize()) {
    Logger::instance().warning("backend initialize returned false");
  }
  sasl_.enable(SaslMechanism::Plain);
  sasl_.enable(SaslMechanism::DigestMd5);
  sasl_.enable(SaslMechanism::Gssapi);
  sasl_.enable(SaslMechanism::External);
  if (config_.enable_ldaps || config_.enable_starttls) {
    tls_.loadCertificate(config_.tls_cert_file, config_.tls_key_file);
  }
  initialized_ = true;
  return true;
}

bool LdapDaemon::start() {
  if (!initialize()) {
    return false;
  }
  if (!listener_.start(config_.listen_address, config_.ldap_port)) {
    Logger::instance().error("failed to listen on " + config_.listen_address + ":" +
                             std::to_string(config_.ldap_port));
    return false;
  }
  running_ = true;
  accept_thread_ = std::thread([this] { acceptLoop(); });
  Logger::instance().info(std::string(kProjectName) + " " + kVersion +
                          " listening on " + config_.listen_address + ":" +
                          std::to_string(boundPort()));
  return true;
}

void LdapDaemon::stop() {
  running_ = false;
  listener_.stop();
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }
}

bool LdapDaemon::running() const { return running_.load(); }

port_t LdapDaemon::boundPort() const { return listener_.boundPort(); }

bool LdapDaemon::testConfig() const {
  std::vector<std::string> errors;
  if (!config_.validateDetailed(errors)) {
    for (const auto &error : errors) {
      Logger::instance().error(error);
    }
    return false;
  }
  return true;
}

void LdapDaemon::acceptLoop() {
  while (running_.load()) {
    auto connection = listener_.acceptConnection(200);
    if (!connection) {
      continue;
    }
    Session session(std::move(*connection), *backend_, config_, running_);
    session.serve();
  }
  Logger::instance().info("simple-ldapd stopped");
}

}  // namespace simple_ldapd
