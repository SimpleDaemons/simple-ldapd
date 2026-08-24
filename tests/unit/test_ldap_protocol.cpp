/**
 * @file test_ldap_protocol.cpp
 * @brief BER, filter, and simple-bind unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/bind.hpp"
#include "simple-ldapd/backend/memory.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/protocol/ber.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/utils/dn.hpp"

#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

bool testBerIntegerRoundtrip() {
  BerWriter writer;
  writer.writeInteger(0);
  writer.writeInteger(127);
  writer.writeInteger(128);
  writer.writeInteger(-1);
  BerReader reader(writer.bytes());
  int64_t zero = 1;
  int64_t small = 0;
  int64_t larger = 0;
  int64_t negative = 0;
  return reader.readInteger(zero) && zero == 0 && reader.readInteger(small) &&
         small == 127 && reader.readInteger(larger) && larger == 128 &&
         reader.readInteger(negative) && negative == -1 && reader.ok();
}

bool testBindMessageRoundtrip() {
  const auto wire = encodeLdapMessage(makeBindRequest(7, "cn=admin,dc=example,dc=com", "secret"));
  const auto decoded = decodeLdapMessage(wire);
  return decoded && decoded->op == ProtocolOp::BindRequest && decoded->message_id == 7 &&
         decoded->bind.simple && decoded->bind.dn == "cn=admin,dc=example,dc=com" &&
         decoded->bind.password == "secret";
}

bool testSearchMessageRoundtrip() {
  SearchRequestData search;
  search.base_dn = "dc=example,dc=com";
  search.scope = SearchScope::Subtree;
  search.filter = SearchFilter::parse("(&(objectClass=inetOrgPerson)(uid=alice))");
  search.attributes = {"cn", "uid"};
  const auto wire = encodeLdapMessage(makeSearchRequest(2, search));
  const auto decoded = decodeLdapMessage(wire);
  return decoded && decoded->op == ProtocolOp::SearchRequest &&
         decoded->search.base_dn == "dc=example,dc=com" &&
         decoded->search.scope == SearchScope::Subtree && decoded->search.filter.valid() &&
         decoded->search.attributes.size() == 2;
}

bool testFilterMatch() {
  DirectoryEntry alice;
  alice.dn = "uid=alice,ou=People,dc=example,dc=com";
  alice.attributes["objectClass"].push_back("inetOrgPerson");
  alice.attributes["uid"].push_back("alice");
  alice.attributes["cn"].push_back("Alice Example");
  const auto present = SearchFilter::parse("(objectClass=*)");
  const auto equality = SearchFilter::parse("(uid=ALICE)");
  const auto combined = SearchFilter::parse("(&(objectClass=inetOrgPerson)(uid=alice))");
  const auto missing = SearchFilter::parse("(uid=bob)");
  const auto negated = SearchFilter::parse("(!(uid=bob))");
  const auto bad = SearchFilter::parse("uid=alice");
  return present.valid() && present.matches(alice) && equality.matches(alice) &&
         combined.matches(alice) && !missing.matches(alice) && negated.matches(alice) &&
         !bad.valid();
}

bool testFilterBerUnknownFails() {
  BerWriter writer;
  writer.writeOctetString(0xA4, "unused");  // substring filter, not implemented
  BerReader reader(writer.bytes());
  SearchFilter filter;
  return !SearchFilter::decodeBer(reader, filter);
}

bool testDnHelpers() {
  return dnEquals("CN=Admin,DC=Example,DC=com", "cn=admin,dc=example,dc=com") &&
         dnEndsWith("uid=alice,ou=People,dc=example,dc=com", "dc=example,dc=com") &&
         dnIsOneLevelChild("ou=People,dc=example,dc=com", "dc=example,dc=com") &&
         !dnIsOneLevelChild("uid=alice,ou=People,dc=example,dc=com", "dc=example,dc=com");
}

bool testSimpleBind() {
  MemoryBackend backend;
  backend.initialize();
  DirectoryEntry alice;
  alice.dn = "uid=alice,dc=example,dc=com";
  alice.attributes["userPassword"].push_back("{CLEARTEXT}alice-secret");
  backend.add(alice);

  LdapConfig config;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  SimpleBindAuthenticator auth(backend, config);
  return auth.bind("", "") == ResultCode::Success &&
         auth.bind("", "x") == ResultCode::InvalidCredentials &&
         auth.bind(config.root_dn, "secret") == ResultCode::Success &&
         auth.bind(config.root_dn, "wrong") == ResultCode::InvalidCredentials &&
         auth.bind(alice.dn, "alice-secret") == ResultCode::Success &&
         auth.bind(alice.dn, "nope") == ResultCode::InvalidCredentials &&
         auth.bind("uid=missing,dc=example,dc=com", "x") == ResultCode::InvalidCredentials;
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
  run("testBerIntegerRoundtrip", testBerIntegerRoundtrip);
  run("testBindMessageRoundtrip", testBindMessageRoundtrip);
  run("testSearchMessageRoundtrip", testSearchMessageRoundtrip);
  run("testFilterMatch", testFilterMatch);
  run("testFilterBerUnknownFails", testFilterBerUnknownFails);
  run("testDnHelpers", testDnHelpers);
  run("testSimpleBind", testSimpleBind);
  std::cout << "Protocol tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
