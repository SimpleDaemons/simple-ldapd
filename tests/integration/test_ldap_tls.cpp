/**
 * @file test_ldap_tls.cpp
 * @brief LDAPS, StartTLS, and confidentiality tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/security/tls.hpp"
#include "simple-ldapd/utils/net.hpp"
#include "simple-ldapd/utils/platform.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace simple_ldapd;

namespace {

bool sendMessage(TcpConnection &connection, const LdapMessage &message) {
  return connection.sendAll(encodeLdapMessage(message));
}

std::optional<LdapMessage> recvMessage(TcpConnection &connection) {
  std::vector<uint8_t> pdu;
  if (!connection.recvPdu(pdu)) {
    return std::nullopt;
  }
  return decodeLdapMessage(pdu);
}

DirectoryEntry makeAlice() {
  DirectoryEntry entry;
  entry.dn = "uid=alice,dc=example,dc=com";
  entry.attributes["objectClass"].push_back("inetOrgPerson");
  entry.attributes["uid"].push_back("alice");
  entry.attributes["cn"].push_back("Alice Example");
  entry.attributes["userPassword"].push_back("alice-secret");
  return entry;
}

DirectoryEntry makeBase() {
  DirectoryEntry entry;
  entry.dn = "dc=example,dc=com";
  entry.attributes["objectClass"].push_back("organization");
  entry.attributes["dc"].push_back("example");
  return entry;
}

bool seed(LdapDaemon &daemon) {
  return daemon.initialize() && daemon.backend() != nullptr &&
         daemon.backend()->add(makeBase()) && daemon.backend()->add(makeAlice());
}

std::string uniqueSuffix() {
#ifndef SIMPLE_LDAPD_WINDOWS
  return std::to_string(getpid());
#else
  return "win";
#endif
}

bool makeSelfSigned(std::string &cert_path, std::string &key_path) {
  const std::string suffix = uniqueSuffix();
  cert_path = "test-simple-ldapd-" + suffix + ".crt";
  key_path = "test-simple-ldapd-" + suffix + ".key";
  std::ostringstream command;
  command << "openssl req -x509 -newkey rsa:2048 -nodes -keyout '" << key_path
          << "' -out '" << cert_path << "' -days 1 -subj '/CN=127.0.0.1' >/dev/null 2>&1";
  return std::system(command.str().c_str()) == 0;
}

bool configureTls(LdapConfig &config, const std::string &cert, const std::string &key) {
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.ldaps_port = 0;
  config.tls_cert_file = cert;
  config.tls_key_file = key;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  return true;
}

#ifndef SIMPLE_LDAPD_SSL
bool skipped() { return true; }
#else

bool testConfidentialityRequiredOnCleartext() {
  std::string cert;
  std::string key;
  if (!makeSelfSigned(cert, key)) {
    return false;
  }
  LdapConfig config;
  configureTls(config, cert, key);
  config.require_confidentiality = true;
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  const bool sent =
      connection && sendMessage(*connection, makeBindRequest(1, config.root_dn, "secret"));
  auto bind = sent ? recvMessage(*connection) : std::nullopt;
  if (connection) {
    connection->close();
  }
  auto anon = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  const bool anon_sent = anon && sendMessage(*anon, makeBindRequest(2, "", ""));
  auto anon_bind = anon_sent ? recvMessage(*anon) : std::nullopt;
  daemon.stop();
  return bind && bind->result == ResultCode::ConfidentialityRequired && anon_bind &&
         anon_bind->result == ResultCode::Success;
}

bool testLdapsBindSearch() {
  std::string cert;
  std::string key;
  if (!makeSelfSigned(cert, key)) {
    return false;
  }
  LdapConfig config;
  configureTls(config, cert, key);
  config.enable_ldaps = true;
  config.require_confidentiality = true;
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start() || daemon.boundLdapsPort() == 0) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundLdapsPort();
  options.ldaps = true;
  options.bind_dn = config.root_dn;
  options.password = "secret";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return static_cast<bool>(client) && error.empty();
}

bool testStartTlsBind() {
  std::string cert;
  std::string key;
  if (!makeSelfSigned(cert, key)) {
    return false;
  }
  LdapConfig config;
  configureTls(config, cert, key);
  config.enable_starttls = true;
  config.require_confidentiality = true;
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start() || daemon.boundPort() == 0) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.starttls = true;
  options.bind_dn = config.root_dn;
  options.password = "secret";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return static_cast<bool>(client) && error.empty();
}

#endif

}  // namespace

int main() {
  int passed = 0;
  int total = 0;
  auto run = [&](const char *name, bool (*fn)()) {
    ++total;
    if (fn()) {
      ++passed;
      std::cout << "PASS " << name << std::endl;
    } else {
      std::cout << "FAIL " << name << std::endl;
    }
  };
#ifndef SIMPLE_LDAPD_SSL
  run("testTlsSkippedWithoutSsl", skipped);
#else
  run("testConfidentialityRequiredOnCleartext", testConfidentialityRequiredOnCleartext);
  run("testLdapsBindSearch", testLdapsBindSearch);
  run("testStartTlsBind", testStartTlsBind);
#endif
  std::cout << "TLS tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
