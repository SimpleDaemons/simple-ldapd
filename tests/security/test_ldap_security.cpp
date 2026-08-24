/**
 * @file test_ldap_security.cpp
 * @brief Security stub tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/rate_limiter.hpp"
#include "simple-ldapd/security/tls.hpp"

#include <iostream>

int main() {
  simple_ldapd::RateLimiter limiter(30);
  if (!limiter.allow("127.0.0.1")) {
    std::cout << "FAIL rate limiter" << std::endl;
    return 1;
  }
  simple_ldapd::TlsContext tls;
  if (tls.enabled()) {
    std::cout << "FAIL tls default" << std::endl;
    return 1;
  }
  std::cout << "PASS security stubs" << std::endl;
  return 0;
}
