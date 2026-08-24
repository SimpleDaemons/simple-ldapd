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

#include <iostream>
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

bool testGssapiHook() {
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
  run("testGssapiHook", testGssapiHook);
  run("testExternalRequiresTls", testExternalRequiresTls);
  run("testPlainConfidentiality", testPlainConfidentiality);
  std::cout << "SASL tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
