/**
 * @file test_ldap_acl.cpp
 * @brief Access-control unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/memory.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/security/acl.hpp"

#include <fstream>
#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

DirectoryEntry makeEntry(const std::string &dn, const std::string &oc) {
  DirectoryEntry entry;
  entry.dn = dn;
  entry.attributes["objectClass"].push_back(oc);
  return entry;
}

bool testParseAclLines() {
  AclRule rule;
  std::string error;
  if (!parseAclLine("anonymous search dc=example,dc=com", rule, error) ||
      rule.who != AclWhoKind::Anonymous || rule.permission != AclPermission::Search ||
      rule.target != "dc=example,dc=com") {
    return false;
  }
  if (!parseAclLine("users write *", rule, error) || rule.who != AclWhoKind::Users ||
      rule.permission != AclPermission::Write || !rule.target.empty()) {
    return false;
  }
  if (!parseAclLine("dn:uid=alice,ou=People,dc=example,dc=com write ou=People,dc=example,dc=com",
                    rule, error) ||
      rule.who != AclWhoKind::Dn || rule.who_value != "uid=alice,ou=People,dc=example,dc=com" ||
      rule.permission != AclPermission::Write) {
    return false;
  }
  if (!parseAclLine("group:cn=admins,ou=Groups,dc=example,dc=com search *", rule, error) ||
      rule.who != AclWhoKind::Group ||
      rule.who_value != "cn=admins,ou=Groups,dc=example,dc=com") {
    return false;
  }
  return parseAclLine("* search", rule, error) && rule.who == AclWhoKind::Anyone &&
         !parseAclLine("anonymous", rule, error);
}

bool testDefaultAllowsSearchDeniesWrite() {
  LdapConfig config;
  config.root_dn = "cn=admin,dc=example,dc=com";
  MemoryBackend backend;
  backend.initialize();
  AccessControl acl(config);
  return acl.maySearch("", "dc=example,dc=com", backend) &&
         !acl.mayWrite("", "dc=example,dc=com", backend) &&
         !acl.mayWrite("uid=alice,dc=example,dc=com", "dc=example,dc=com", backend) &&
         acl.mayWrite("cn=admin,dc=example,dc=com", "dc=example,dc=com", backend);
}

bool testUsersSearchDeniesAnonymous() {
  LdapConfig config;
  config.root_dn = "cn=admin,dc=example,dc=com";
  AclRule rule;
  std::string error;
  if (!parseAclLine("users search dc=example,dc=com", rule, error)) {
    return false;
  }
  config.acls.push_back(rule);
  MemoryBackend backend;
  backend.initialize();
  AccessControl acl(config);
  return !acl.maySearch("", "dc=example,dc=com", backend) &&
         acl.maySearch("uid=alice,dc=example,dc=com", "dc=example,dc=com", backend) &&
         acl.maySearch("", "", backend) &&
         !acl.mayWrite("uid=alice,dc=example,dc=com", "ou=People,dc=example,dc=com", backend);
}

bool testDnWriteOnSubtree() {
  LdapConfig config;
  AclRule rule;
  std::string error;
  parseAclLine("dn:uid=alice,ou=People,dc=example,dc=com write ou=People,dc=example,dc=com",
               rule, error);
  config.acls.push_back(rule);
  MemoryBackend backend;
  backend.initialize();
  AccessControl acl(config);
  const std::string alice = "uid=alice,ou=People,dc=example,dc=com";
  return acl.mayWrite(alice, "uid=bob,ou=People,dc=example,dc=com", backend) &&
         acl.maySearch(alice, "uid=bob,ou=People,dc=example,dc=com", backend) &&
         !acl.mayWrite(alice, "dc=example,dc=com", backend);
}

bool testGroupMembership() {
  LdapConfig config;
  AclRule rule;
  std::string error;
  parseAclLine("group:cn=admins,ou=Groups,dc=example,dc=com write ou=People,dc=example,dc=com",
               rule, error);
  config.acls.push_back(rule);
  MemoryBackend backend;
  backend.initialize();
  auto alice = makeEntry("uid=alice,ou=People,dc=example,dc=com", "inetOrgPerson");
  alice.attributes["uid"].push_back("alice");
  auto group = makeEntry("cn=admins,ou=Groups,dc=example,dc=com", "groupOfNames");
  group.attributes["member"].push_back("uid=alice,ou=People,dc=example,dc=com");
  backend.add(alice);
  backend.add(group);
  AccessControl acl(config);
  return acl.mayWrite("uid=alice,ou=People,dc=example,dc=com",
                      "uid=bob,ou=People,dc=example,dc=com", backend) &&
         !acl.mayWrite("uid=bob,ou=People,dc=example,dc=com",
                       "uid=bob,ou=People,dc=example,dc=com", backend);
}

bool testConfigAclLines() {
  const std::string path = "test-simple-ldapd-acl.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "acl = users search dc=example,dc=com\n";
  out << "acl = dn:uid=alice,ou=People,dc=example,dc=com write ou=People,dc=example,dc=com\n";
  out.close();
  LdapConfig config;
  return config.loadFromFile(path) && config.validate() && config.acls.size() == 2 &&
         config.acls[0].who == AclWhoKind::Users && config.acls[1].who == AclWhoKind::Dn;
}

bool testInvalidAclFailsValidate() {
  const std::string path = "test-simple-ldapd-bad-acl.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "acl = nope search *\n";
  out.close();
  LdapConfig config;
  return config.loadFromFile(path) && !config.acl_errors.empty() && !config.validate();
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
  run("testParseAclLines", testParseAclLines);
  run("testDefaultAllowsSearchDeniesWrite", testDefaultAllowsSearchDeniesWrite);
  run("testUsersSearchDeniesAnonymous", testUsersSearchDeniesAnonymous);
  run("testDnWriteOnSubtree", testDnWriteOnSubtree);
  run("testGroupMembership", testGroupMembership);
  run("testConfigAclLines", testConfigAclLines);
  run("testInvalidAclFailsValidate", testInvalidAclFailsValidate);
  std::cout << "ACL tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
