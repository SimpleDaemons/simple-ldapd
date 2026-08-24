/**
 * @file test_ldap_ops.cpp
 * @brief Compare, Who Am I, paged results, and typesOnly tests
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

#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace simple_ldapd;

namespace {

DirectoryEntry makePerson(const std::string &uid) {
  DirectoryEntry entry;
  entry.dn = "uid=" + uid + ",dc=example,dc=com";
  entry.attributes["objectClass"].push_back("inetOrgPerson");
  entry.attributes["uid"].push_back(uid);
  entry.attributes["cn"].push_back(uid);
  entry.attributes["sn"].push_back("Example");
  entry.attributes["userPassword"].push_back(uid + "-secret");
  return entry;
}

bool seed(LdapDaemon &daemon) {
  DirectoryEntry base;
  base.dn = "dc=example,dc=com";
  base.attributes["objectClass"].push_back("organization");
  base.attributes["dc"].push_back("example");
  return daemon.initialize() && daemon.backend() != nullptr && daemon.backend()->add(base) &&
         daemon.backend()->add(makePerson("alice")) && daemon.backend()->add(makePerson("bob")) &&
         daemon.backend()->add(makePerson("carol"));
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

bool testCompare() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  auto client = cli::LdapClient::openBound(
      [](port_t port) {
        cli::ClientOptions options;
        options.host = "127.0.0.1";
        options.port = port;
        options.bind_dn = "cn=admin,dc=example,dc=com";
        options.password = "secret";
        return options;
      }(daemon.boundPort()),
      error);
  if (!client) {
    daemon.stop();
    return false;
  }
  CompareRequestData yes;
  yes.dn = "uid=alice,dc=example,dc=com";
  yes.attribute = "uid";
  yes.value = "alice";
  CompareRequestData no;
  no.dn = yes.dn;
  no.attribute = "uid";
  no.value = "bob";
  CompareRequestData password;
  password.dn = yes.dn;
  password.attribute = "userPassword";
  password.value = "alice-secret";
  const ResultCode matched = client->compare(yes);
  const ResultCode missed = client->compare(no);
  const ResultCode secret = client->compare(password);
  client->unbind();
  daemon.stop();
  return matched == ResultCode::CompareTrue && missed == ResultCode::CompareFalse &&
         secret == ResultCode::CompareTrue;
}

bool testWhoAmI() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.bind_dn = "alice";
  options.password = "alice-secret";
  std::string error;
  auto client = cli::LdapClient::openBound(options, error);
  if (!client) {
    daemon.stop();
    return false;
  }
  std::string authzid;
  const ResultCode result = client->whoAmI(authzid);
  client->unbind();
  daemon.stop();
  return result == ResultCode::Success && authzid == "dn:uid=alice,dc=example,dc=com";
}

bool testPagedAndTypesOnly() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection ||
      !sendMessage(*connection, makeBindRequest(1, "cn=admin,dc=example,dc=com", "secret"))) {
    daemon.stop();
    return false;
  }
  auto bind = recvMessage(*connection);
  SearchRequestData search;
  search.base_dn = "dc=example,dc=com";
  search.filter = SearchFilter::parse("(objectClass=inetOrgPerson)");
  search.types_only = true;
  LdapMessage page1 = makeSearchRequest(2, search);
  page1.controls.push_back(makePagedResultsControl(1, {}));
  if (!bind || bind->result != ResultCode::Success || !sendMessage(*connection, page1)) {
    daemon.stop();
    return false;
  }
  auto entry1 = recvMessage(*connection);
  auto done1 = recvMessage(*connection);
  std::string cookie;
  int remaining = 0;
  if (!entry1 || entry1->op != ProtocolOp::SearchResultEntry || entry1->entry.attributes.empty() ||
      !entry1->entry.attributes.front().values.empty() || !done1 || done1->controls.empty() ||
      !decodePagedResultsValue(done1->controls.front().value, remaining, cookie) || cookie.empty()) {
    daemon.stop();
    return false;
  }
  LdapMessage page2 = makeSearchRequest(3, search);
  page2.controls.push_back(makePagedResultsControl(10, cookie));
  if (!sendMessage(*connection, page2)) {
    daemon.stop();
    return false;
  }
  int entries = 0;
  std::string last_cookie = "x";
  while (true) {
    auto message = recvMessage(*connection);
    if (!message) {
      daemon.stop();
      return false;
    }
    if (message->op == ProtocolOp::SearchResultEntry) {
      ++entries;
      continue;
    }
    if (message->op == ProtocolOp::SearchResultDone) {
      last_cookie.clear();
      if (!message->controls.empty()) {
        decodePagedResultsValue(message->controls.front().value, remaining, last_cookie);
      }
      break;
    }
    daemon.stop();
    return false;
  }
  sendMessage(*connection, makeUnbindRequest(4));
  daemon.stop();
  return entries >= 1 && last_cookie.empty();
}

bool testRootDseAdvertisesOps() {
  LdapDaemon daemon(testConfig());
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection || !sendMessage(*connection, makeBindRequest(1, "", ""))) {
    daemon.stop();
    return false;
  }
  recvMessage(*connection);
  SearchRequestData search;
  search.scope = SearchScope::Base;
  search.filter = SearchFilter::present("objectClass");
  search.attributes = {"supportedExtension", "supportedControl"};
  if (!sendMessage(*connection, makeSearchRequest(2, search))) {
    daemon.stop();
    return false;
  }
  auto entry = recvMessage(*connection);
  daemon.stop();
  if (!entry || entry->op != ProtocolOp::SearchResultEntry) {
    return false;
  }
  bool who = false;
  bool paged = false;
  for (const auto &attribute : entry->entry.attributes) {
    for (const auto &value : attribute.values) {
      if (value == kWhoAmIOid) {
        who = true;
      }
      if (value == kPagedResultsOid) {
        paged = true;
      }
    }
  }
  return who && paged;
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
  run("testCompare", testCompare);
  run("testWhoAmI", testWhoAmI);
  run("testPagedAndTypesOnly", testPagedAndTypesOnly);
  run("testRootDseAdvertisesOps", testRootDseAdvertisesOps);
  std::cout << "Ops tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
