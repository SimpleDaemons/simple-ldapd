/**
 * @file test_ldap_write.cpp
 * @brief TCP add/modify/delete integration tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/sqlite.hpp"
#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/security/acl.hpp"

#include <cstdio>
#include <iostream>

using namespace simple_ldapd;

namespace {

bool seedBase(LdapDaemon &daemon) {
  DirectoryEntry base;
  base.dn = "dc=example,dc=com";
  base.attributes["objectClass"].push_back("organization");
  base.attributes["dc"].push_back("example");
  DirectoryEntry people;
  people.dn = "ou=People,dc=example,dc=com";
  people.attributes["objectClass"].push_back("organizationalUnit");
  people.attributes["ou"].push_back("People");
  return daemon.initialize() && daemon.backend() != nullptr && daemon.backend()->add(base) &&
         daemon.backend()->add(people);
}

bool testWriteOps() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  LdapDaemon daemon(config);
  if (!seedBase(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.bind_dn = config.root_dn;
  options.password = config.root_password;
  auto client = cli::LdapClient::openBound(options, error);
  if (!client) {
    daemon.stop();
    return false;
  }

  DirectoryEntry bob;
  bob.dn = "uid=bob,ou=People,dc=example,dc=com";
  bob.attributes["objectClass"].push_back("inetOrgPerson");
  bob.attributes["uid"].push_back("bob");
  bob.attributes["cn"].push_back("Bob Example");
  bob.attributes["sn"].push_back("Example");
  if (client->add(bob) != ResultCode::Success) {
    daemon.stop();
    return false;
  }
  if (client->modify(bob.dn, {{ModifyOp::Replace, "mail", {"bob@example.com"}}}) !=
      ResultCode::Success) {
    daemon.stop();
    return false;
  }
  ModifyDnRequestData rename;
  rename.dn = bob.dn;
  rename.new_rdn = "uid=robert";
  rename.delete_old_rdn = true;
  if (client->modifyDn(rename) != ResultCode::Success) {
    daemon.stop();
    return false;
  }
  const std::string renamed = "uid=robert,ou=People,dc=example,dc=com";
  auto found = daemon.backend()->lookup(renamed);
  const bool renamed_ok = found && found->attributes.count("mail") == 1 &&
                          !daemon.backend()->lookup(bob.dn);
  if (client->del(renamed) != ResultCode::Success) {
    daemon.stop();
    return false;
  }
  client->unbind();
  daemon.stop();
  return renamed_ok && !daemon.backend()->lookup(renamed);
}

bool testAnonymousCannotWrite() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.root_password = "secret";
  LdapDaemon daemon(config);
  if (!seedBase(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  auto client = cli::LdapClient::openBound(options, error);
  if (!client) {
    daemon.stop();
    return false;
  }
  DirectoryEntry bob;
  bob.dn = "uid=bob,ou=People,dc=example,dc=com";
  bob.attributes["uid"].push_back("bob");
  const ResultCode result = client->add(bob);
  client->unbind();
  daemon.stop();
  return result == ResultCode::InsufficientAccessRights;
}

bool testAclWriteByDn() {
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  AclRule search;
  AclRule write;
  std::string error;
  parseAclLine("users search dc=example,dc=com", search, error);
  parseAclLine("dn:uid=alice,ou=People,dc=example,dc=com write ou=People,dc=example,dc=com",
               write, error);
  config.acls.push_back(search);
  config.acls.push_back(write);
  LdapDaemon daemon(config);
  DirectoryEntry alice;
  alice.dn = "uid=alice,ou=People,dc=example,dc=com";
  alice.attributes["objectClass"].push_back("inetOrgPerson");
  alice.attributes["uid"].push_back("alice");
  alice.attributes["cn"].push_back("Alice Example");
  alice.attributes["sn"].push_back("Example");
  alice.attributes["userPassword"].push_back("alice-secret");
  if (!seedBase(daemon) || !daemon.backend()->add(alice) || !daemon.start()) {
    return false;
  }
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.bind_dn = alice.dn;
  options.password = "alice-secret";
  auto client = cli::LdapClient::openBound(options, error);
  if (!client) {
    daemon.stop();
    return false;
  }
  DirectoryEntry bob;
  bob.dn = "uid=bob,ou=People,dc=example,dc=com";
  bob.attributes["objectClass"].push_back("inetOrgPerson");
  bob.attributes["uid"].push_back("bob");
  bob.attributes["cn"].push_back("Bob Example");
  bob.attributes["sn"].push_back("Example");
  const ResultCode added = client->add(bob);
  const ResultCode denied = client->del("dc=example,dc=com");
  client->unbind();
  daemon.stop();
  return added == ResultCode::Success && denied == ResultCode::InsufficientAccessRights &&
         daemon.backend()->lookup(bob.dn).has_value();
}

#ifdef SIMPLE_LDAPD_SQLITE
bool testSqliteLiveDurability() {
  const std::string db = "test-write-sqlite.sqlite";
  std::remove(db.c_str());
  std::remove((db + "-wal").c_str());
  std::remove((db + "-shm").c_str());
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.backend = "sqlite";
  config.sqlite_file = db;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  {
    LdapDaemon daemon(config);
    if (!seedBase(daemon) || daemon.backend() == nullptr ||
        daemon.backend()->name() != "sqlite" || !daemon.start()) {
      return false;
    }
    std::string error;
    cli::ClientOptions options;
    options.host = "127.0.0.1";
    options.port = daemon.boundPort();
    options.bind_dn = config.root_dn;
    options.password = config.root_password;
    auto client = cli::LdapClient::openBound(options, error);
    if (!client) {
      daemon.stop();
      return false;
    }
    DirectoryEntry bob;
    bob.dn = "uid=bob,ou=People,dc=example,dc=com";
    bob.attributes["objectClass"].push_back("inetOrgPerson");
    bob.attributes["uid"].push_back("bob");
    bob.attributes["cn"].push_back("Bob Example");
    bob.attributes["sn"].push_back("Example");
    if (client->add(bob) != ResultCode::Success) {
      daemon.stop();
      return false;
    }
    client->unbind();
    daemon.stop();
  }
  SqliteBackend reopened(db);
  if (!reopened.initialize()) {
    return false;
  }
  const auto found = reopened.lookup("uid=bob,ou=People,dc=example,dc=com");
  std::remove(db.c_str());
  std::remove((db + "-wal").c_str());
  std::remove((db + "-shm").c_str());
  return found && found->attributes.count("uid") == 1;
}
#else
bool testSqliteLiveDurability() { return true; }
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
  run("testWriteOps", testWriteOps);
  run("testAnonymousCannotWrite", testAnonymousCannotWrite);
  run("testAclWriteByDn", testAclWriteByDn);
  run("testSqliteLiveDurability", testSqliteLiveDurability);
  std::cout << "Write tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
