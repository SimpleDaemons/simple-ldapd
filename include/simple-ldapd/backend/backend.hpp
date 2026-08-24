/**
 * @file backend.hpp
 * @brief Pluggable directory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace simple_ldapd {

struct DirectoryEntry {
  std::string dn;
  std::map<std::string, std::vector<std::string>> attributes;
};

enum class SearchScope { Base = 0, OneLevel = 1, Subtree = 2 };

class SearchFilter;

class Backend {
public:
  virtual ~Backend() = default;

  virtual bool initialize() = 0;
  virtual std::string name() const = 0;

  virtual std::optional<DirectoryEntry> lookup(const std::string &dn) const = 0;
  virtual std::vector<DirectoryEntry> search(const std::string &base_dn,
                                             SearchScope scope,
                                             const SearchFilter &filter) const = 0;
  virtual bool add(const DirectoryEntry &entry) = 0;
  virtual bool modify(const DirectoryEntry &entry) = 0;
  virtual bool remove(const std::string &dn) = 0;
};

}  // namespace simple_ldapd
