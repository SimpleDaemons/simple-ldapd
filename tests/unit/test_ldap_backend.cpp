/**
 * @file test_ldap_backend.cpp
 * @brief Backend unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/sasl.hpp"
#include "simple-ldapd/backend/memory.hpp"
#include "simple-ldapd/backend/sqlite.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/schema/registry.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>

using namespace simple_ldapd;

namespace {

bool testMemoryBackend() {
  MemoryBackend backend;
  if (!backend.initialize()) {
    return false;
  }
  DirectoryEntry entry;
  entry.dn = "uid=alice,dc=example,dc=com";
  entry.attributes["uid"].push_back("alice");
  if (!backend.add(entry)) {
    return false;
  }
  if (backend.add(entry)) {
    return false;
  }
  auto found = backend.lookup(entry.dn);
  if (!found || found->attributes["uid"].empty()) {
    return false;
  }
  auto matches =
      backend.search("dc=example,dc=com", SearchScope::Subtree, SearchFilter::present("uid"));
  return matches.size() == 1 && backend.remove(entry.dn);
}

bool testSchemaLoad() {
  const std::string path = "test-core.schema";
  std::ofstream out(path);
  out << "attributetype ( 2.5.4.3 NAME 'cn' )\n";
  out << "objectclass ( 2.5.6.6 NAME 'person' )\n";
  out.close();
  SchemaRegistry registry;
  if (!registry.loadFile(path)) {
    return false;
  }
  return registry.hasAttribute("cn") && registry.hasObjectClass("person");
}

bool testSaslAdvertise() {
  SaslAuthenticator sasl;
  sasl.enable(SaslMechanism::Gssapi);
  sasl.enable(SaslMechanism::External);
  auto names = sasl.advertised();
  return names.size() == 2;
}

void removeSqliteFiles(const std::string &path) {
  std::remove(path.c_str());
  std::remove((path + "-wal").c_str());
  std::remove((path + "-shm").c_str());
}

#ifdef SIMPLE_LDAPD_SQLITE
bool testSqliteBackend() {
  const std::string db = "test-backend.sqlite";
  const std::string seed = "test-backend-seed.ldif";
  const std::string exported = "test-backend-export.ldif";
  removeSqliteFiles(db);
  std::remove(seed.c_str());
  std::remove(exported.c_str());

  {
    SqliteBackend backend(db);
    if (!backend.initialize() || backend.name() != "sqlite") {
      return false;
    }
    DirectoryEntry people;
    people.dn = "ou=People,dc=example,dc=com";
    people.attributes["ou"].push_back("People");
    DirectoryEntry alice;
    alice.dn = "uid=alice,ou=People,dc=example,dc=com";
    alice.attributes["uid"].push_back("alice");
    alice.attributes["cn"].push_back("Alice");
    if (!backend.add(people) || !backend.add(alice) || backend.add(alice)) {
      return false;
    }
    auto found = backend.lookup("UID=ALICE,OU=PEOPLE,DC=EXAMPLE,DC=COM");
    if (!found || found->attributes["uid"].empty() || found->dn != alice.dn) {
      return false;
    }
    auto matches =
        backend.search("dc=example,dc=com", SearchScope::Subtree, SearchFilter::present("uid"));
    if (matches.size() != 1 || !backend.hasChildren("ou=People,dc=example,dc=com")) {
      return false;
    }
    alice.attributes["mail"].push_back("alice@example.com");
    if (!backend.modify(alice)) {
      return false;
    }
    if (!backend.rename(alice.dn, "uid=alicia,ou=People,dc=example,dc=com")) {
      return false;
    }
    if (backend.lookup(alice.dn) ||
        !backend.lookup("uid=alicia,ou=People,dc=example,dc=com")) {
      return false;
    }
    if (!backend.remove("uid=alicia,ou=People,dc=example,dc=com") ||
        backend.lookup("uid=alicia,ou=People,dc=example,dc=com")) {
      return false;
    }
    backend.persist();
    if (!backend.exportFile(exported)) {
      return false;
    }
  }

  {
    SqliteBackend reopened(db);
    if (!reopened.initialize()) {
      return false;
    }
    if (!reopened.lookup("ou=People,dc=example,dc=com") ||
        reopened.lookup("uid=alice,ou=People,dc=example,dc=com")) {
      return false;
    }
  }

  {
    std::ofstream out(seed);
    out << "dn: dc=example,dc=com\n";
    out << "objectClass: organization\n";
    out << "dc: example\n";
    out.close();
    removeSqliteFiles("test-backend-seed.sqlite");
    SqliteBackend seeded("test-backend-seed.sqlite", seed);
    if (!seeded.initialize()) {
      return false;
    }
    if (!seeded.lookup("dc=example,dc=com")) {
      return false;
    }
    DirectoryEntry extra;
    extra.dn = "ou=Groups,dc=example,dc=com";
    extra.attributes["ou"].push_back("Groups");
    if (!seeded.add(extra)) {
      return false;
    }
  }
  {
    SqliteBackend seeded("test-backend-seed.sqlite", seed);
    if (!seeded.initialize() || !seeded.lookup("ou=Groups,dc=example,dc=com")) {
      return false;
    }
  }

  removeSqliteFiles(db);
  removeSqliteFiles("test-backend-seed.sqlite");
  std::remove(seed.c_str());
  std::remove(exported.c_str());
  return true;
}
#else
bool testSqliteBackend() { return true; }
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
  run("testMemoryBackend", testMemoryBackend);
  run("testSchemaLoad", testSchemaLoad);
  run("testSaslAdvertise", testSaslAdvertise);
  run("testSqliteBackend", testSqliteBackend);
  std::cout << "Backend tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
