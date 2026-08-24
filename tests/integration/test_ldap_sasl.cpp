/**
 * @file test_ldap_sasl.cpp
 * @brief SASL bind and Root DSE tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/sasl.hpp"
#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/utils/net.hpp"
#include "simple-ldapd/utils/platform.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace simple_ldapd;

namespace {

bool seed(LdapDaemon &daemon) {
  DirectoryEntry base;
  base.dn = "dc=example,dc=com";
  base.attributes["objectClass"].push_back("organization");
  base.attributes["dc"].push_back("example");
  DirectoryEntry alice;
  alice.dn = "uid=alice,dc=example,dc=com";
  alice.attributes["objectClass"].push_back("inetOrgPerson");
  alice.attributes["uid"].push_back("alice");
  alice.attributes["cn"].push_back("Alice Example");
  alice.attributes["sAMAccountName"].push_back("alice");
  alice.attributes["userPassword"].push_back("alice-secret");
  return daemon.initialize() && daemon.backend() != nullptr && daemon.backend()->add(base) &&
         daemon.backend()->add(alice);
}

LdapConfig testConfig() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.base_dn = "dc=example,dc=com";
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  return config;
}

std::string writeLabKeytab() {
  const std::string path = "test-simple-ldapd-sasl.keytab";
  std::ofstream out(path);
  out << "realm = EXAMPLE.COM\n";
  out << "service = ldap/localhost\n";
  out << "key = lab-service-secret\n";
  out.close();
  return path;
}

LdapConfig gssapiConfig() {
  auto config = testConfig();
  config.krb_realm = "EXAMPLE.COM";
  config.gssapi_keytab = writeLabKeytab();
  return config;
}

bool hasValue(const SearchEntryData &entry, const std::string &name, const std::string &wanted) {
  for (const auto &attribute : entry.attributes) {
    if (attribute.type != name) {
      continue;
    }
    for (const auto &value : attribute.values) {
      if (value == wanted) {
        return true;
      }
    }
  }
  return false;
}

bool testRootDseAdvertisesSasl() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection ||
      !connection->sendAll(encodeLdapMessage(makeBindRequest(1, "", "")))) {
    daemon.stop();
    return false;
  }
  std::vector<uint8_t> pdu;
  if (!connection->recvPdu(pdu)) {
    daemon.stop();
    return false;
  }
  SearchRequestData search;
  search.scope = SearchScope::Base;
  search.filter = SearchFilter::present("objectClass");
  search.attributes = {"supportedSASLMechanisms", "namingContexts"};
  if (!connection->sendAll(encodeLdapMessage(makeSearchRequest(2, search)))) {
    daemon.stop();
    return false;
  }
  if (!connection->recvPdu(pdu)) {
    daemon.stop();
    return false;
  }
  auto entry = decodeLdapMessage(pdu);
  if (!connection->recvPdu(pdu)) {
    daemon.stop();
    return false;
  }
  daemon.stop();
  return entry && entry->op == ProtocolOp::SearchResultEntry &&
         hasValue(entry->entry, "supportedSASLMechanisms", "PLAIN") &&
         hasValue(entry->entry, "supportedSASLMechanisms", "DIGEST-MD5") &&
         hasValue(entry->entry, "supportedSASLMechanisms", "EXTERNAL") &&
         hasValue(entry->entry, "supportedSASLMechanisms", "GSSAPI") &&
         hasValue(entry->entry, "namingContexts", "dc=example,dc=com");
}

bool testPlainBindByUid() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.sasl_mechanism = "PLAIN";
  options.sasl_authcid = "alice";
  options.password = "alice-secret";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return static_cast<bool>(client) && error.empty();
}

bool testDigestMd5Bind() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.sasl_mechanism = "DIGEST-MD5";
  options.sasl_authcid = "alice";
  options.password = "alice-secret";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return static_cast<bool>(client) && error.empty();
}

bool testGssapiWithoutKeytab() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection ||
      !connection->sendAll(encodeLdapMessage(
          makeSaslBindRequest(1, "", "GSSAPI", "ticket")))) {
    daemon.stop();
    return false;
  }
  std::vector<uint8_t> pdu;
  if (!connection->recvPdu(pdu)) {
    daemon.stop();
    return false;
  }
  auto response = decodeLdapMessage(pdu);
  daemon.stop();
  return response && response->result == ResultCode::AuthMethodNotSupported;
}

bool testGssapiTicketBind() {
  auto config = gssapiConfig();
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.sasl_mechanism = "GSSAPI";
  options.sasl_authcid = "alice";
  options.keytab = config.gssapi_keytab;
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return static_cast<bool>(client) && error.empty();
}

bool testGssapiUnknownPrincipal() {
  auto config = gssapiConfig();
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.sasl_mechanism = "GSSAPI";
  options.sasl_authcid = "mallory";
  options.keytab = config.gssapi_keytab;
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return !client && error == "invalidCredentials";
}

bool testExternalRequiresTls() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection ||
      !connection->sendAll(encodeLdapMessage(
          makeSaslBindRequest(1, "", "EXTERNAL", "uid=alice,dc=example,dc=com")))) {
    daemon.stop();
    return false;
  }
  std::vector<uint8_t> pdu;
  if (!connection->recvPdu(pdu)) {
    daemon.stop();
    return false;
  }
  auto response = decodeLdapMessage(pdu);
  daemon.stop();
  return response && response->result == ResultCode::ConfidentialityRequired;
}

bool testPlainConfidentiality() {
  auto config = testConfig();
  config.require_confidentiality = true;
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.sasl_mechanism = "PLAIN";
  options.sasl_authcid = "alice";
  options.password = "alice-secret";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return !client && error == "confidentialityRequired";
}

#ifdef SIMPLE_LDAPD_SSL
std::string uniqueSuffix() {
#ifndef SIMPLE_LDAPD_WINDOWS
  return std::to_string(getpid());
#else
  return "win";
#endif
}

bool makeCaSignedPair(const std::string &ca_crt, const std::string &ca_key, const std::string &cn,
                      int serial, std::string &cert_path, std::string &key_path) {
  const std::string suffix = uniqueSuffix();
  cert_path = "test-simple-ldapd-" + cn + "-" + suffix + ".crt";
  key_path = "test-simple-ldapd-" + cn + "-" + suffix + ".key";
  const std::string csr = "test-simple-ldapd-" + cn + "-" + suffix + ".csr";
  std::ostringstream command;
  command << "openssl req -newkey rsa:2048 -nodes -keyout '" << key_path << "' -out '" << csr
          << "' -subj '/CN=" << cn << "' >/dev/null 2>&1 && openssl x509 -req -in '" << csr
          << "' -CA '" << ca_crt << "' -CAkey '" << ca_key << "' -set_serial " << serial
          << " -out '" << cert_path << "' -days 1 >/dev/null 2>&1";
  return std::system(command.str().c_str()) == 0;
}

bool makeExternalTlsMaterial(std::string &ca_crt, std::string &server_crt, std::string &server_key,
                             std::string &client_crt, std::string &client_key) {
  const std::string suffix = uniqueSuffix();
  ca_crt = "test-simple-ldapd-ca-" + suffix + ".crt";
  const std::string ca_key = "test-simple-ldapd-ca-" + suffix + ".key";
  std::ostringstream ca;
  ca << "openssl req -x509 -newkey rsa:2048 -nodes -keyout '" << ca_key << "' -out '" << ca_crt
     << "' -days 1 -subj '/CN=test-ca' >/dev/null 2>&1";
  if (std::system(ca.str().c_str()) != 0) {
    return false;
  }
  return makeCaSignedPair(ca_crt, ca_key, "127.0.0.1", 1, server_crt, server_key) &&
         makeCaSignedPair(ca_crt, ca_key, "alice", 2, client_crt, client_key);
}

LdapConfig externalTlsConfig(const std::string &ca_crt, const std::string &server_crt,
                             const std::string &server_key) {
  auto config = testConfig();
  config.enable_ldaps = true;
  config.ldaps_port = 0;
  config.tls_cert_file = server_crt;
  config.tls_key_file = server_key;
  config.tls_ca_file = ca_crt;
  return config;
}

bool testExternalTlsWithoutClientCert() {
  std::string ca_crt;
  std::string server_crt;
  std::string server_key;
  std::string client_crt;
  std::string client_key;
  if (!makeExternalTlsMaterial(ca_crt, server_crt, server_key, client_crt, client_key)) {
    return false;
  }
  LdapDaemon daemon(externalTlsConfig(ca_crt, server_crt, server_key));
  if (!seed(daemon) || !daemon.start() || daemon.boundLdapsPort() == 0) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundLdapsPort();
  options.ldaps = true;
  options.ca_file = ca_crt;
  options.sasl_mechanism = "EXTERNAL";
  options.sasl_authcid = "alice";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return !client && error == "invalidCredentials";
}

bool testExternalWithClientCert() {
  std::string ca_crt;
  std::string server_crt;
  std::string server_key;
  std::string client_crt;
  std::string client_key;
  if (!makeExternalTlsMaterial(ca_crt, server_crt, server_key, client_crt, client_key)) {
    return false;
  }
  LdapDaemon daemon(externalTlsConfig(ca_crt, server_crt, server_key));
  if (!seed(daemon) || !daemon.start() || daemon.boundLdapsPort() == 0) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundLdapsPort();
  options.ldaps = true;
  options.ca_file = ca_crt;
  options.tls_cert_file = client_crt;
  options.tls_key_file = client_key;
  options.sasl_mechanism = "EXTERNAL";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return static_cast<bool>(client) && error.empty();
}

bool testExternalMismatchedAuthzid() {
  std::string ca_crt;
  std::string server_crt;
  std::string server_key;
  std::string client_crt;
  std::string client_key;
  if (!makeExternalTlsMaterial(ca_crt, server_crt, server_key, client_crt, client_key)) {
    return false;
  }
  LdapDaemon daemon(externalTlsConfig(ca_crt, server_crt, server_key));
  if (!seed(daemon) || !daemon.start() || daemon.boundLdapsPort() == 0) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundLdapsPort();
  options.ldaps = true;
  options.ca_file = ca_crt;
  options.tls_cert_file = client_crt;
  options.tls_key_file = client_key;
  options.sasl_mechanism = "EXTERNAL";
  options.sasl_authcid = "mallory";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  daemon.stop();
  return !client && error == "invalidCredentials";
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
  run("testRootDseAdvertisesSasl", testRootDseAdvertisesSasl);
  run("testPlainBindByUid", testPlainBindByUid);
  run("testDigestMd5Bind", testDigestMd5Bind);
  run("testGssapiWithoutKeytab", testGssapiWithoutKeytab);
  run("testGssapiTicketBind", testGssapiTicketBind);
  run("testGssapiUnknownPrincipal", testGssapiUnknownPrincipal);
  run("testExternalRequiresTls", testExternalRequiresTls);
  run("testPlainConfidentiality", testPlainConfidentiality);
#ifdef SIMPLE_LDAPD_SSL
  run("testExternalTlsWithoutClientCert", testExternalTlsWithoutClientCert);
  run("testExternalWithClientCert", testExternalWithClientCert);
  run("testExternalMismatchedAuthzid", testExternalMismatchedAuthzid);
#endif
  std::cout << "SASL tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
