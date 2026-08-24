/**
 * @file test_ldap_password.cpp
 * @brief userPassword hashing and userAccountControl tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/password.hpp"
#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/utils/dn.hpp"

#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

bool startsWithIgnoreCase(const std::string &value, const std::string &prefix) {
  if (value.size() < prefix.size()) {
    return false;
  }
  return iequals(value.substr(0, prefix.size()), prefix);
}

bool testSshaRoundTrip() {
  const std::string stored = encodeUserPassword("alice-secret");
  if (!startsWithIgnoreCase(stored, "{SSHA}") || stored == "alice-secret") {
    return false;
  }
  if (!verifyUserPassword(stored, "alice-secret") ||
      verifyUserPassword(stored, "wrong-secret")) {
    return false;
  }
  return !recoverablePassword(stored);
}

bool testCleartextAndPlain() {
  return verifyUserPassword("{CLEARTEXT}alice-secret", "alice-secret") &&
         verifyUserPassword("alice-secret", "alice-secret") &&
         !verifyUserPassword("{CLEARTEXT}alice-secret", "nope") &&
         recoverablePassword("{CLEARTEXT}lab") == std::string("lab") &&
         recoverablePassword("lab") == std::string("lab") &&
         encodeUserPassword("{CLEARTEXT}lab") == "{CLEARTEXT}lab";
}

bool testShaScheme() {
  return verifyUserPassword("{SHA}5en6G6MezRroT3XKqkdPOmY/BfQ=", "secret") &&
         !verifyUserPassword("{SHA}5en6G6MezRroT3XKqkdPOmY/BfQ=", "wrong");
}

bool testAccountDisabled() {
  DirectoryEntry entry;
  entry.dn = "uid=alice,dc=example,dc=com";
  if (accountDisabled(entry)) {
    return false;
  }
  entry.attributes["userAccountControl"].push_back("512");
  if (accountDisabled(entry)) {
    return false;
  }
  entry.attributes["userAccountControl"].front() = "514";
  if (!accountDisabled(entry)) {
    return false;
  }
  entry.attributes["userAccountControl"].front() = "2";
  return accountDisabled(entry);
}

bool testEncodeSkipsDelete() {
  std::vector<AttributeModification> changes;
  AttributeModification del;
  del.op = ModifyOp::Delete;
  del.type = "userPassword";
  del.values.push_back("alice-secret");
  AttributeModification add;
  add.op = ModifyOp::Replace;
  add.type = "userPassword";
  add.values.push_back("alice-secret");
  changes.push_back(del);
  changes.push_back(add);
  encodeUserPasswordChanges(changes);
  return changes[0].values.front() == "alice-secret" &&
         startsWithIgnoreCase(changes[1].values.front(), "{SSHA}");
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
  run("testSshaRoundTrip", testSshaRoundTrip);
  run("testCleartextAndPlain", testCleartextAndPlain);
  run("testShaScheme", testShaScheme);
  run("testAccountDisabled", testAccountDisabled);
  run("testEncodeSkipsDelete", testEncodeSkipsDelete);
  std::cout << "Password storage tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
