/**
 * @file test_ldap_config.cpp
 * @brief Configuration unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include "simple-ldapd/version.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

std::string writeTempConfig() {
  const std::string path = "test-simple-ldapd.conf";
  std::ofstream out(path);
  out << "listen_address = 127.0.0.1\n";
  out << "ldap_port = 3389\n";
  out << "backend = memory\n";
  out << "base_dn = dc=example,dc=com\n";
  return path;
}

bool testDefaultConfig() {
  LdapConfig config;
  assert(config.ldap_port == kLdapDefaultPort);
  assert(config.ldaps_port == kLdapsDefaultPort);
  assert(config.backend == "memory");
  assert(config.validate());
  return true;
}

bool testInvalidPort() {
  LdapConfig config;
  config.ldap_port = 0;
  return !config.validate();
}

bool testFileLoad() {
  const auto path = writeTempConfig();
  LdapConfig config;
  if (!config.loadFromFile(path)) {
    return false;
  }
  return config.ldap_port == 3389 && config.listen_address == "127.0.0.1";
}

bool testFilterParse() {
  auto ok = SearchFilter::parse("(objectClass=*)");
  auto bad = SearchFilter::parse("objectClass=*");
  return ok.valid() && !bad.valid();
}

bool testResultCodes() {
  return std::string(toString(ResultCode::Unavailable)) == "unavailable" &&
         kVersion[0] != '\0';
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
  run("testDefaultConfig", testDefaultConfig);
  run("testInvalidPort", testInvalidPort);
  run("testFileLoad", testFileLoad);
  run("testFilterParse", testFilterParse);
  run("testResultCodes", testResultCodes);
  std::cout << "Config tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
