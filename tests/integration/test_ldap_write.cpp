/**
 * @file test_ldap_write.cpp
 * @brief TCP add/modify/delete integration tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"
#include "simple-ldapd/protocol/filter.hpp"

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
  std::cout << "Write tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
