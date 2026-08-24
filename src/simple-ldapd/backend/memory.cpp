/**
 * @file memory.cpp
 * @brief In-memory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/memory.hpp"

#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/utils/dn.hpp"

namespace simple_ldapd {

bool MemoryBackend::initialize() { return true; }

std::optional<DirectoryEntry> MemoryBackend::lookup(const std::string &dn) const {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto &pair : entries_) {
    if (dnEquals(pair.first, dn)) {
      return pair.second;
    }
  }
  return std::nullopt;
}

std::vector<DirectoryEntry> MemoryBackend::search(const std::string &base_dn,
                                                  SearchScope scope,
                                                  const SearchFilter &filter) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<DirectoryEntry> matches;
  for (const auto &pair : entries_) {
    const std::string &dn = pair.first;
    bool in_scope = false;
    switch (scope) {
    case SearchScope::Base:
      in_scope = dnEquals(dn, base_dn);
      break;
    case SearchScope::OneLevel:
      in_scope = dnIsOneLevelChild(dn, base_dn);
      break;
    case SearchScope::Subtree:
      in_scope = dnEndsWith(dn, base_dn);
      break;
    }
    if (in_scope && filter.matches(pair.second)) {
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
  for (const auto &pair : entries_) {
    if (dnEquals(pair.first, entry.dn)) {
      return false;
    }
  }
  entries_[entry.dn] = entry;
  return true;
}

bool MemoryBackend::modify(const DirectoryEntry &entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &pair : entries_) {
    if (dnEquals(pair.first, entry.dn)) {
      pair.second = entry;
      pair.second.dn = entry.dn;
      return true;
    }
  }
  return false;
}

bool MemoryBackend::remove(const std::string &dn) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    if (dnEquals(it->first, dn)) {
      entries_.erase(it);
      return true;
    }
  }
  return false;
}

}  // namespace simple_ldapd
