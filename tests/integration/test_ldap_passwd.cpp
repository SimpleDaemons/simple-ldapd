/**
 * @file test_ldap_passwd.cpp
 * @brief RFC 3062 password modify tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/utils/net.hpp"

#include <fstream>
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
  alice.attributes["sn"].push_back("Example");
  alice.attributes["userPassword"].push_back("alice-secret");
  DirectoryEntry bob;
  bob.dn = "uid=bob,dc=example,dc=com";
  bob.attributes["objectClass"].push_back("inetOrgPerson");
  bob.attributes["uid"].push_back("bob");
  bob.attributes["cn"].push_back("Bob Example");
  bob.attributes["sn"].push_back("Example");
  bob.attributes["userPassword"].push_back("bob-secret");
  return daemon.initialize() && daemon.backend() != nullptr && daemon.backend()->add(base) &&
         daemon.backend()->add(alice) && daemon.backend()->add(bob);
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

cli::ClientOptions bound(const LdapDaemon &daemon, const std::string &dn,
                         const std::string &password) {
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.bind_dn = dn;
  options.password = password;
  return options;
}

bool canBind(port_t port, const std::string &dn, const std::string &password) {
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = port;
  options.bind_dn = dn;
  options.password = password;
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  return static_cast<bool>(client);
}

bool hasExtension(const SearchEntryData &entry, const std::string &oid) {
  for (const auto &attribute : entry.attributes) {
    if (attribute.type != "supportedExtension") {
      continue;
    }
    for (const auto &value : attribute.values) {
      if (value == oid) {
        return true;
      }
    }
  }
  return false;
}

bool testRootDseAdvertisesPasswordModify() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection || !connection->sendAll(encodeLdapMessage(makeBindRequest(1, "", "")))) {
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
  search.attributes = {"supportedExtension"};
  if (!connection->sendAll(encodeLdapMessage(makeSearchRequest(2, search)))) {
    daemon.stop();
    return false;
  }
  if (!connection->recvPdu(pdu)) {
    daemon.stop();
    return false;
  }
  auto entry = decodeLdapMessage(pdu);
  daemon.stop();
  return entry && entry->op == ProtocolOp::SearchResultEntry &&
         hasExtension(entry->entry, kPasswordModifyOid);
}

bool testRootSetsPassword() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  auto client = cli::LdapClient::openBound(bound(daemon, "cn=admin,dc=example,dc=com", "secret"),
                                           error);
  if (!client) {
    daemon.stop();
    return false;
  }
  PasswordModifyRequest request;
  request.user_identity = "alice";
  request.new_password = "alice-new";
  const ResultCode changed = client->passwordModify(request);
  client->unbind();
  const bool bound_new = canBind(daemon.boundPort(), "uid=alice,dc=example,dc=com", "alice-new");
  const bool bound_old = canBind(daemon.boundPort(), "uid=alice,dc=example,dc=com", "alice-secret");
  daemon.stop();
  return changed == ResultCode::Success && bound_new && !bound_old;
}

bool testSelfChangePassword() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  auto client = cli::LdapClient::openBound(
      bound(daemon, "uid=alice,dc=example,dc=com", "alice-secret"), error);
  if (!client) {
    daemon.stop();
    return false;
  }
  PasswordModifyRequest request;
  request.new_password = "alice-self";
  const ResultCode changed = client->passwordModify(request);
  client->unbind();
  const bool ok = canBind(daemon.boundPort(), "uid=alice,dc=example,dc=com", "alice-self");
  daemon.stop();
  return changed == ResultCode::Success && ok;
}

bool testCannotChangeAnotherPassword() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  auto client = cli::LdapClient::openBound(
      bound(daemon, "uid=alice,dc=example,dc=com", "alice-secret"), error);
  if (!client) {
    daemon.stop();
    return false;
  }
  PasswordModifyRequest request;
  request.user_identity = "uid=bob,dc=example,dc=com";
  request.new_password = "stolen";
  const ResultCode changed = client->passwordModify(request);
  client->unbind();
  daemon.stop();
  return changed == ResultCode::InsufficientAccessRights;
}

bool testWrongOldPassword() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  auto client = cli::LdapClient::openBound(
      bound(daemon, "uid=alice,dc=example,dc=com", "alice-secret"), error);
  if (!client) {
    daemon.stop();
    return false;
  }
  PasswordModifyRequest request;
  request.old_password = "nope";
  request.new_password = "alice-self";
  const ResultCode changed = client->passwordModify(request);
  client->unbind();
  daemon.stop();
  return changed == ResultCode::InvalidCredentials;
}

bool testPasswordModifyRequiresConfidentiality() {
  auto config = testConfig();
  config.require_confidentiality = true;
  config.krb_realm = "EXAMPLE.COM";
  config.gssapi_keytab = "test-simple-ldapd-passwd.keytab";
  std::ofstream out(config.gssapi_keytab);
  out << "realm = EXAMPLE.COM\n";
  out << "service = ldap/localhost\n";
  out << "key = lab-service-secret\n";
  out.close();
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
  if (!client) {
    daemon.stop();
    return false;
  }
  PasswordModifyRequest request;
  request.new_password = "alice-new";
  const ResultCode changed = client->passwordModify(request);
  client->unbind();
  daemon.stop();
  return changed == ResultCode::ConfidentialityRequired;
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
  run("testRootDseAdvertisesPasswordModify", testRootDseAdvertisesPasswordModify);
  run("testRootSetsPassword", testRootSetsPassword);
  run("testSelfChangePassword", testSelfChangePassword);
  run("testCannotChangeAnotherPassword", testCannotChangeAnotherPassword);
  run("testWrongOldPassword", testWrongOldPassword);
  run("testPasswordModifyRequiresConfidentiality", testPasswordModifyRequiresConfidentiality);
  std::cout << "Password tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
