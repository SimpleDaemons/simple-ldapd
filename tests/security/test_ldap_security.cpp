/**
 * @file test_ldap_security.cpp
 * @brief Rate limiter and TLS context tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/rate_limiter.hpp"
#include "simple-ldapd/security/tls.hpp"

#include <iostream>

int main() {
  int passed = 0;
  int total = 0;
  auto run = [&](const char *name, bool ok) {
    ++total;
    if (ok) {
      ++passed;
      std::cout << "PASS " << name << std::endl;
    } else {
      std::cout << "FAIL " << name << std::endl;
    }
  };

  simple_ldapd::RateLimiter unlimited(0);
  run("testUnlimitedRateLimiter", unlimited.allow("127.0.0.1") && unlimited.allow("127.0.0.1"));

  simple_ldapd::RateLimiter limiter(2);
  const bool first = limiter.allow("127.0.0.1:1111");
  const bool second = limiter.allow("127.0.0.1:2222");
  const bool third = limiter.allow("127.0.0.1:3333");
  const bool other = limiter.allow("10.0.0.1:1");
  run("testBindRateLimiterWindow", first && second && !third && other);

  simple_ldapd::TlsContext tls;
  run("testTlsDefaultDisabled", !tls.enabled());

  std::cout << "Security tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
