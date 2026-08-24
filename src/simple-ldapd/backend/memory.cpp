/**
 * @file memory.cpp
 * @brief In-memory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/memory.hpp"

namespace simple_ldapd {

bool MemoryBackend::initialize() { return true; }

std::optional<DirectoryEntry> MemoryBackend::lookup(const std::string &dn) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = entries_.find(dn);
  if (it == entries_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<DirectoryEntry> MemoryBackend::search(const std::string &base_dn,
                                                  const std::string & /*filter*/) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<DirectoryEntry> matches;
  for (const auto &pair : entries_) {
    if (pair.first.size() >= base_dn.size() &&
        pair.first.compare(pair.first.size() - base_dn.size(), base_dn.size(),
                           base_dn) == 0) {
      matches.push_back(pair.second);
    }
  }
  return matches;
}

bool MemoryBackend::add(const DirectoryEntry &entry) {
  if (entry.dn.empty()) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (entries_.count(entry.dn) != 0) {
    return false;
  }
  entries_[entry.dn] = entry;
  return true;
}

bool MemoryBackend::modify(const DirectoryEntry &entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = entries_.find(entry.dn);
  if (it == entries_.end()) {
    return false;
  }
  it->second = entry;
  return true;
}

bool MemoryBackend::remove(const std::string &dn) {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.erase(dn) > 0;
}

}  // namespace simple_ldapd
