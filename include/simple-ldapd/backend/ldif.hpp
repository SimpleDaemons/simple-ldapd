/**
 * @file ldif.hpp
 * @brief LDIF import/export backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/memory.hpp"
#include <mutex>
#include <string>

namespace simple_ldapd {

class LdifBackend : public MemoryBackend {
public:
  explicit LdifBackend(std::string path);

  bool initialize() override;
  std::string name() const override { return "ldif"; }

  bool importFile(const std::string &path);
  bool exportFile(const std::string &path) const;
  void persist() override;

private:
  std::string path_;
  mutable std::mutex persist_mutex_;
};

}  // namespace simple_ldapd
