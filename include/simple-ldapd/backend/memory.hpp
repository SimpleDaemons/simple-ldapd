/**
 * @file memory.hpp
 * @brief In-memory directory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include <mutex>

namespace simple_ldapd {

class MemoryBackend : public Backend {
public:
  bool initialize() override;
  std::string name() const override { return "memory"; }

  std::optional<DirectoryEntry> lookup(const std::string &dn) const override;
  std::vector<DirectoryEntry> search(const std::string &base_dn,
                                     const std::string &filter) const override;
  bool add(const DirectoryEntry &entry) override;
  bool modify(const DirectoryEntry &entry) override;
  bool remove(const std::string &dn) override;

private:
  mutable std::mutex mutex_;
  std::map<std::string, DirectoryEntry> entries_;
};

}  // namespace simple_ldapd
