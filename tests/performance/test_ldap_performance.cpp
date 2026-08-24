/**
 * @file test_ldap_performance.cpp
 * @brief Backend smoke performance test
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/memory.hpp"

#include <chrono>
#include <iostream>
#include <string>

int main() {
  simple_ldapd::MemoryBackend backend;
  backend.initialize();
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < 1000; ++i) {
    simple_ldapd::DirectoryEntry entry;
    entry.dn = "uid=user" + std::to_string(i) + ",dc=example,dc=com";
    backend.add(entry);
  }
  const auto elapsed = std::chrono::steady_clock::now() - start;
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  std::cout << "Inserted 1000 entries in " << ms << " ms" << std::endl;
  return ms < 5000 ? 0 : 1;
}
