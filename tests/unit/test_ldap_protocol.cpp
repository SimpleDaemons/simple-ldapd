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
  const auto initial = SearchFilter::parse("(cn=ALI*)");
  const auto any = SearchFilter::parse("(cn=*Exam*)");
  const auto final_part = SearchFilter::parse("(cn=*example)");
  const auto both = SearchFilter::parse("(cn=A*e)");
  const auto miss = SearchFilter::parse("(cn=bob*)");
  const auto bad = SearchFilter::parse("uid=alice");
  return present.valid() && present.matches(alice) && equality.matches(alice) &&
         combined.matches(alice) && !missing.matches(alice) && negated.matches(alice) &&
         initial.valid() && initial.root().type == FilterType::Substring &&
         initial.matches(alice) && any.matches(alice) && final_part.matches(alice) &&
         both.matches(alice) && !miss.matches(alice) && !bad.valid();
}

bool testSubstringFilterRoundtrip() {
  const auto parsed = SearchFilter::parse("(cn=Ali*Ex*ple)");
  const auto wire = parsed.encodeBer();
  BerReader reader(wire);
  SearchFilter decoded;
  return parsed.valid() && parsed.root().initial == "Ali" && parsed.root().any.size() == 1 &&
         parsed.root().any.front() == "Ex" && parsed.root().final == "ple" &&
         SearchFilter::decodeBer(reader, decoded) && decoded.root().type == FilterType::Substring &&
         decoded.root().attribute == "cn" && decoded.root().initial == "Ali" &&
         decoded.root().any.size() == 1 && decoded.root().any.front() == "Ex" &&
         decoded.root().final == "ple";
}

bool testDnHelpers() {
  return dnEquals("CN=Admin,DC=Example,DC=com", "cn=admin,dc=example,dc=com") &&
         dnEndsWith("uid=alice,ou=People,dc=example,dc=com", "dc=example,dc=com") &&
         dnIsOneLevelChild("ou=People,dc=example,dc=com", "dc=example,dc=com") &&
         !dnIsOneLevelChild("uid=alice,ou=People,dc=example,dc=com", "dc=example,dc=com") &&
         dnParent("uid=alice,ou=People,dc=example,dc=com") == "ou=People,dc=example,dc=com" &&
         dnRdn("uid=alice,ou=People,dc=example,dc=com") == "uid=alice";
}

bool testAddModifyMessages() {
  AddRequestData add;
  add.dn = "uid=bob,dc=example,dc=com";
  add.attributes.push_back({"uid", {"bob"}});
  const auto add_wire = encodeLdapMessage(makeAddRequest(3, add));
  const auto add_decoded = decodeLdapMessage(add_wire);
  ModifyRequestData modify;
  modify.dn = add.dn;
  modify.changes.push_back({ModifyOp::Replace, "mail", {"bob@example.com"}});
  const auto modify_wire = encodeLdapMessage(makeModifyRequest(4, modify));
  const auto modify_decoded = decodeLdapMessage(modify_wire);
  const auto del_decoded = decodeLdapMessage(encodeLdapMessage(makeDelRequest(5, add.dn)));
  const auto add_result =
      decodeLdapMessage(encodeLdapMessage(makeLdapResult(6, ProtocolOp::AddResponse,
                                                        ResultCode::EntryAlreadyExists, "exists")));
  return add_decoded && add_decoded->add.dn == add.dn &&
         modify_decoded && modify_decoded->modify.changes.size() == 1 && del_decoded &&
         del_decoded->delete_dn == add.dn && add_result &&
         add_result->op == ProtocolOp::AddResponse &&
         add_result->result == ResultCode::EntryAlreadyExists;
}

bool testApplyModifications() {
  DirectoryEntry entry;
  entry.dn = "uid=bob,dc=example,dc=com";
  entry.attributes["cn"].push_back("Bob");
  if (applyModifications(entry, {{ModifyOp::Add, "mail", {"bob@example.com"}}}) !=
      ResultCode::Success) {
    return false;
  }
  if (applyModifications(entry, {{ModifyOp::Replace, "cn", {"Robert"}}}) != ResultCode::Success) {
    return false;
  }
  return entry.attributes["mail"].size() == 1 && entry.attributes["cn"].front() == "Robert" &&
         applyModifications(entry, {{ModifyOp::Delete, "mail", {}}}) == ResultCode::Success &&
         entry.attributes.count("mail") == 0;
}

bool testSimpleBind() {
  MemoryBackend backend;
  backend.initialize();
  DirectoryEntry alice;
  alice.dn = "uid=alice,ou=People,dc=example,dc=com";
  alice.attributes["uid"].push_back("alice");
  alice.attributes["sAMAccountName"].push_back("alice");
  alice.attributes["userPassword"].push_back("{CLEARTEXT}alice-secret");
  backend.add(alice);

  LdapConfig config;
  config.base_dn = "dc=example,dc=com";
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  SimpleBindAuthenticator auth(backend, config);
  return auth.bind("", "") == ResultCode::Success &&
         auth.bind("", "x") == ResultCode::InvalidCredentials &&
         auth.bind(config.root_dn, "secret") == ResultCode::Success &&
         auth.bind(config.root_dn, "wrong") == ResultCode::InvalidCredentials &&
         auth.bind(alice.dn, "alice-secret") == ResultCode::Success &&
         auth.bind("uid=alice,dc=example,dc=com", "alice-secret") == ResultCode::Success &&
         auth.bind("alice", "alice-secret") == ResultCode::Success &&
         auth.bind(alice.dn, "nope") == ResultCode::InvalidCredentials &&
         auth.bind("uid=missing,dc=example,dc=com", "x") == ResultCode::InvalidCredentials &&
         auth.resolveName("uid=alice,dc=example,dc=com") == alice.dn;
}

bool testExtendedRequestRoundtrip() {
  const auto wire =
      encodeLdapMessage(makeExtendedRequest(9, "1.3.6.1.4.1.1466.20037"));
  const auto decoded = decodeLdapMessage(wire);
  const auto result =
      decodeLdapMessage(encodeLdapMessage(makeLdapResult(
          9, ProtocolOp::ExtendedResponse, ResultCode::Success)));
  return decoded && decoded->op == ProtocolOp::ExtendedRequest && decoded->message_id == 9 &&
         decoded->extended_oid == "1.3.6.1.4.1.1466.20037" && result &&
         result->op == ProtocolOp::ExtendedResponse && result->result == ResultCode::Success;
}

bool testPasswordModifyRoundtrip() {
  PasswordModifyRequest request;
  request.user_identity = "uid=alice,dc=example,dc=com";
  request.old_password = "old-secret";
  request.new_password = "new-secret";
  const auto decoded = decodeLdapMessage(encodeLdapMessage(makePasswordModifyRequest(3, request)));
  PasswordModifyRequest parsed;
  return decoded && decoded->op == ProtocolOp::ExtendedRequest &&
         decoded->extended_oid == kPasswordModifyOid &&
         decodePasswordModifyValue(decoded->extended_value, parsed) &&
         parsed.user_identity == request.user_identity && parsed.old_password == "old-secret" &&
         parsed.new_password == "new-secret";
}

bool testCompareAndPagedRoundtrip() {
  CompareRequestData compare;
  compare.dn = "uid=alice,dc=example,dc=com";
  compare.attribute = "uid";
  compare.value = "alice";
  const auto compared = decodeLdapMessage(encodeLdapMessage(makeCompareRequest(5, compare)));
  LdapMessage search = makeSearchRequest(6, SearchRequestData{});
  search.controls.push_back(makePagedResultsControl(2, "3"));
  const auto paged = decodeLdapMessage(encodeLdapMessage(search));
  int size = 0;
  std::string cookie;
  LdapMessage who = makeLdapResult(7, ProtocolOp::ExtendedResponse, ResultCode::Success);
  who.extended_oid = kWhoAmIOid;
  who.extended_value = "dn:uid=alice,dc=example,dc=com";
  const auto who_decoded = decodeLdapMessage(encodeLdapMessage(who));
  return compared && compared->op == ProtocolOp::CompareRequest &&
         compared->compare.dn == compare.dn && compared->compare.attribute == "uid" &&
         compared->compare.value == "alice" && paged && paged->controls.size() == 1 &&
         paged->controls.front().oid == kPagedResultsOid &&
         decodePagedResultsValue(paged->controls.front().value, size, cookie) && size == 2 &&
         cookie == "3" && who_decoded && who_decoded->extended_oid == kWhoAmIOid &&
         who_decoded->extended_value == "dn:uid=alice,dc=example,dc=com";
}

bool testSaslBindRoundtrip() {
  const std::string creds = std::string(1, '\0') + "alice" + std::string(1, '\0') + "secret";
  const auto request = decodeLdapMessage(
      encodeLdapMessage(makeSaslBindRequest(4, "", "PLAIN", creds)));
  const auto response = decodeLdapMessage(
      encodeLdapMessage(makeBindResponse(4, ResultCode::SaslBindInProgress, "", "nonce")));
  return request && !request->bind.simple && request->bind.sasl_mechanism == "PLAIN" &&
         request->bind.sasl_credentials == creds && response &&
         response->result == ResultCode::SaslBindInProgress &&
         response->server_sasl_creds == "nonce";
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
  run("testSubstringFilterRoundtrip", testSubstringFilterRoundtrip);
  run("testDnHelpers", testDnHelpers);
  run("testSimpleBind", testSimpleBind);
  run("testAddModifyMessages", testAddModifyMessages);
  run("testApplyModifications", testApplyModifications);
  run("testExtendedRequestRoundtrip", testExtendedRequestRoundtrip);
  run("testPasswordModifyRoundtrip", testPasswordModifyRoundtrip);
  run("testCompareAndPagedRoundtrip", testCompareAndPagedRoundtrip);
  run("testSaslBindRoundtrip", testSaslBindRoundtrip);
  std::cout << "Protocol tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
