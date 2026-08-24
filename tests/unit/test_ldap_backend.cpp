/**
 * @file test_ldap_backend.cpp
 * @brief Backend unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/sasl.hpp"
#include "simple-ldapd/backend/memory.hpp"
#include "simple-ldapd/schema/registry.hpp"

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
  auto matches = backend.search("dc=example,dc=com", "(objectClass=*)");
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
  std::cout << "Backend tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
