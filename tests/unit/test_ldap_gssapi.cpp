/**
 * @file test_ldap_gssapi.cpp
 * @brief Lab GSSAPI ticket unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/gssapi.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

GssapiKeytab sampleKeytab() {
  GssapiKeytab keytab;
  keytab.realm = "EXAMPLE.COM";
  keytab.service = "ldap/localhost";
  keytab.key = "lab-service-secret";
  return keytab;
}

bool testRealmFromBase() {
  return kerberosRealmFromBase("dc=example,dc=com") == "EXAMPLE.COM" &&
         normalizeKerberosPrincipal("alice@EXAMPLE.COM", "EXAMPLE.COM") == "alice" &&
         normalizeKerberosPrincipal("alice@OTHER.COM", "EXAMPLE.COM").empty();
}

bool testMintAndVerify() {
  const auto keytab = sampleKeytab();
  const std::string ticket = mintGssapiTicket(keytab, "alice@EXAMPLE.COM");
  std::string principal;
  std::string error;
  return !ticket.empty() && verifyGssapiTicket(keytab, ticket, principal, error) &&
         principal == "alice" && error.empty();
}

bool testTamperedTicketFails() {
  const auto keytab = sampleKeytab();
  std::string ticket = mintGssapiTicket(keytab, "alice");
  const auto pos = ticket.find("alice");
  if (pos == std::string::npos) {
    return false;
  }
  ticket.replace(pos, 5, "mallory");
  std::string principal;
  std::string error;
  return !verifyGssapiTicket(keytab, ticket, principal, error);
}

bool testExpiredTicketFails() {
  const auto keytab = sampleKeytab();
  const auto now = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                                 std::chrono::system_clock::now().time_since_epoch())
                                                 .count());
  const std::string ticket = encodeGssapiTicket(keytab, "alice", now - 180, now - 120);
  std::string principal;
  std::string error;
  return !ticket.empty() && !verifyGssapiTicket(keytab, ticket, principal, error);
}

bool testLoadKeytab() {
  const std::string path = "test-simple-ldapd.keytab";
  std::ofstream out(path);
  out << "realm = example.com\n";
  out << "service = ldap/localhost\n";
  out << "key = lab-service-secret\n";
  out.close();
  GssapiKeytab keytab;
  std::string error;
  return loadGssapiKeytab(path, keytab, error) && keytab.realm == "EXAMPLE.COM" &&
         keytab.service == "ldap/localhost" && keytab.key == "lab-service-secret";
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
  run("testRealmFromBase", testRealmFromBase);
  run("testMintAndVerify", testMintAndVerify);
  run("testTamperedTicketFails", testTamperedTicketFails);
  run("testExpiredTicketFails", testExpiredTicketFails);
  run("testLoadKeytab", testLoadKeytab);
  std::cout << "GSSAPI tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
