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
#include "simple-ldapd/utils/logger.hpp"
#include "simple-ldapd/version.hpp"
#include <string>

namespace simple_ldapd {

LdapDaemon::LdapDaemon(LdapConfig config) : config_(std::move(config)) {}

bool LdapDaemon::initialize() {
  if (!config_.validate()) {
    Logger::instance().error("invalid configuration");
    return false;
  }
  if (!config_.log_file.empty()) {
    Logger::instance().setLogFile(config_.log_file);
  }
  schema_.loadDirectory(config_.schema_dir);
  if (config_.backend == "ldif") {
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
  return true;
}

bool LdapDaemon::start() {
  if (!initialize()) {
    return false;
  }
  running_ = true;
  const bool bound = listener_.start(config_.listen_address, config_.ldap_port);
  if (!bound) {
    Logger::instance().warning(
        "listener bind skipped or failed; protocol codec is still a stub");
  }
  Logger::instance().info(std::string(kProjectName) + " " + kVersion +
                          " running (LDAPv3 not implemented yet)");
  return true;
}

void LdapDaemon::stop() {
  running_ = false;
  listener_.stop();
  Logger::instance().info("simple-ldapd stopped");
}

bool LdapDaemon::running() const { return running_; }

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

}  // namespace simple_ldapd
