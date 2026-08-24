/**
 * @file test_ldap_bind_search.cpp
 * @brief TCP bind/search integration tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

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

bool hasAttribute(const SearchEntryData &entry, const std::string &name) {
  for (const auto &attribute : entry.attributes) {
    if (attribute.type == name) {
      return true;
    }
  }
  return false;
}

bool testAnonymousSearch() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.root_password = "secret";
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start() || daemon.boundPort() == 0) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection) {
    daemon.stop();
    return false;
  }
  if (!sendMessage(*connection, makeBindRequest(1, "", ""))) {
    daemon.stop();
    return false;
  }
  auto bind = recvMessage(*connection);
  if (!bind || bind->result != ResultCode::Success) {
    daemon.stop();
    return false;
  }
  SearchRequestData search;
  search.base_dn = "dc=example,dc=com";
  search.filter = SearchFilter::parse("(uid=alice)");
  if (!sendMessage(*connection, makeSearchRequest(2, search))) {
    daemon.stop();
    return false;
  }
  auto entry = recvMessage(*connection);
  auto done = recvMessage(*connection);
  sendMessage(*connection, makeUnbindRequest(3));
  daemon.stop();
  return entry && entry->op == ProtocolOp::SearchResultEntry &&
         entry->entry.dn == "uid=alice,dc=example,dc=com" &&
         !hasAttribute(entry->entry, "userPassword") && done &&
         done->op == ProtocolOp::SearchResultDone && done->result == ResultCode::Success;
}

bool testRootSeesPasswordAndUserBind() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection ||
      !sendMessage(*connection, makeBindRequest(1, config.root_dn, "secret"))) {
    daemon.stop();
    return false;
  }
  auto bind = recvMessage(*connection);
  SearchRequestData search;
  search.base_dn = "dc=example,dc=com";
  search.filter = SearchFilter::present("uid");
  const bool sent = bind && bind->result == ResultCode::Success &&
                    sendMessage(*connection, makeSearchRequest(2, search));
  auto entry = sent ? recvMessage(*connection) : std::nullopt;
  auto done = sent ? recvMessage(*connection) : std::nullopt;
  sendMessage(*connection, makeUnbindRequest(3));
  connection->close();

  auto user = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  const bool user_ok =
      user && sendMessage(*user, makeBindRequest(1, "uid=alice,dc=example,dc=com",
                                                 "alice-secret"));
  auto user_bind = user_ok ? recvMessage(*user) : std::nullopt;
  if (user) {
    sendMessage(*user, makeUnbindRequest(2));
  }

  auto bad = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  const bool bad_sent =
      bad && sendMessage(*bad, makeBindRequest(1, config.root_dn, "wrong"));
  auto bad_bind = bad_sent ? recvMessage(*bad) : std::nullopt;
  daemon.stop();
  return entry && hasAttribute(entry->entry, "userPassword") && done &&
         done->result == ResultCode::Success && user_bind &&
         user_bind->result == ResultCode::Success && bad_bind &&
         bad_bind->result == ResultCode::InvalidCredentials;
}

bool testSubstringSearch() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  LdapDaemon daemon(config);
  if (!seed(daemon) || !daemon.start()) {
    return false;
  }
  auto connection = TcpConnection::connectTo("127.0.0.1", daemon.boundPort());
  if (!connection || !sendMessage(*connection, makeBindRequest(1, "", ""))) {
    daemon.stop();
    return false;
  }
  auto bind = recvMessage(*connection);
  SearchRequestData search;
  search.base_dn = "dc=example,dc=com";
  search.filter = SearchFilter::parse("(cn=Ali*)");
  if (!bind || bind->result != ResultCode::Success ||
      !sendMessage(*connection, makeSearchRequest(2, search))) {
    daemon.stop();
    return false;
  }
  auto entry = recvMessage(*connection);
  auto done = recvMessage(*connection);
  sendMessage(*connection, makeUnbindRequest(3));
  daemon.stop();
  return entry && entry->op == ProtocolOp::SearchResultEntry &&
         entry->entry.dn == "uid=alice,dc=example,dc=com" && done &&
         done->op == ProtocolOp::SearchResultDone && done->result == ResultCode::Success;
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
  run("testAnonymousSearch", testAnonymousSearch);
  run("testRootSeesPasswordAndUserBind", testRootSeesPasswordAndUserBind);
  run("testSubstringSearch", testSubstringSearch);
  std::cout << "Bind/search tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
