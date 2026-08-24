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
#include "simple-ldapd/backend/sqlite.hpp"
#include "simple-ldapd/core/session.hpp"
#include "simple-ldapd/utils/logger.hpp"
#include "simple-ldapd/version.hpp"
#include <string>
#include <vector>

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
  if (config_.backend == "sqlite") {
    backend_ = std::make_unique<SqliteBackend>(config_.sqlite_file, config_.ldif_file);
  } else if (!config_.ldif_file.empty() || config_.backend == "ldif") {
    backend_ = std::make_unique<LdifBackend>(config_.ldif_file);
  } else {
    backend_ = std::make_unique<MemoryBackend>();
  }
  if (!backend_->initialize()) {
    Logger::instance().warning("backend initialize returned false");
    if (config_.backend == "sqlite") {
      Logger::instance().error("sqlite backend failed to initialize");
      return false;
    }
  }
  sasl_.enable(SaslMechanism::Plain);
  sasl_.enable(SaslMechanism::DigestMd5);
  sasl_.enable(SaslMechanism::Gssapi);
  sasl_.enable(SaslMechanism::External);
  if (config_.enable_ldaps || config_.enable_starttls) {
    if (!tls_.loadCertificate(config_.tls_cert_file, config_.tls_key_file)) {
      Logger::instance().error("failed to load TLS certificate");
      return false;
    }
    if (!config_.tls_ca_file.empty() && !tls_.loadCa(config_.tls_ca_file)) {
      Logger::instance().warning("failed to load TLS CA file");
    }
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
  if (config_.enable_ldaps) {
    if (!ldaps_listener_.start(config_.listen_address, config_.ldaps_port)) {
      Logger::instance().error("failed to listen for LDAPS on " + config_.listen_address +
                               ":" + std::to_string(config_.ldaps_port));
      listener_.stop();
      return false;
    }
  }
  running_ = true;
  accept_thread_ = std::thread([this] { acceptLoop(); });
  Logger::instance().info(std::string(kProjectName) + " " + kVersion +
                          " listening on " + config_.listen_address + ":" +
                          std::to_string(boundPort()) +
                          (config_.enable_ldaps
                               ? (" ldaps:" + std::to_string(boundLdapsPort()))
                               : ""));
  return true;
}

void LdapDaemon::stop() {
  running_ = false;
  listener_.stop();
  ldaps_listener_.stop();
  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }
  joinWorkers();
}

bool LdapDaemon::running() const { return running_.load(); }

port_t LdapDaemon::boundPort() const { return listener_.boundPort(); }

port_t LdapDaemon::boundLdapsPort() const { return ldaps_listener_.boundPort(); }

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

void LdapDaemon::serveConnection(TcpConnection connection, bool ldaps) {
  if (ldaps && !connection.handshakeTls(tls_, true)) {
    return;
  }
  Session session(std::move(connection), *backend_, config_, running_, &tls_, &schema_,
                  &sasl_);
  session.serve();
}

void LdapDaemon::launchSession(TcpConnection connection, bool ldaps) {
  reapWorkers();
  auto finished = std::make_shared<std::atomic<bool>>(false);
  SessionWorker worker;
  worker.finished = finished;
  worker.thread =
      std::thread([this, connection = std::move(connection), ldaps, finished]() mutable {
        serveConnection(std::move(connection), ldaps);
        finished->store(true);
      });
  std::lock_guard<std::mutex> lock(workers_mutex_);
  workers_.push_back(std::move(worker));
}

void LdapDaemon::reapWorkers() {
  std::vector<std::thread> finished;
  {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    auto it = workers_.begin();
    while (it != workers_.end()) {
      if (it->finished && it->finished->load()) {
        finished.push_back(std::move(it->thread));
        it = workers_.erase(it);
      } else {
        ++it;
      }
    }
  }
  for (auto &thread : finished) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void LdapDaemon::joinWorkers() {
  std::vector<std::thread> live;
  {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    for (auto &worker : workers_) {
      live.push_back(std::move(worker.thread));
    }
    workers_.clear();
  }
  for (auto &thread : live) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void LdapDaemon::acceptLoop() {
  while (running_.load()) {
    auto connection = listener_.acceptConnection(100);
    if (connection) {
      launchSession(std::move(*connection), false);
    }
    if (config_.enable_ldaps) {
      auto tls_connection = ldaps_listener_.acceptConnection(100);
      if (tls_connection) {
        launchSession(std::move(*tls_connection), true);
      }
    }
  }
  Logger::instance().info("simple-ldapd stopped");
}

}  // namespace simple_ldapd
