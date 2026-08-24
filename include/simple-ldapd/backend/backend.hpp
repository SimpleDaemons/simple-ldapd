/**
 * @file backend.hpp
 * @brief Pluggable directory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/protocol/result_codes.hpp"
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

enum class ModifyOp { Add = 0, Delete = 1, Replace = 2 };

struct AttributeModification {
  ModifyOp op{ModifyOp::Replace};
  std::string type;
  std::vector<std::string> values;
};

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
  virtual bool rename(const std::string &from, const std::string &to) = 0;
  virtual bool hasChildren(const std::string &dn) const = 0;
  virtual void persist() {}
};

ResultCode applyModifications(DirectoryEntry &entry,
                              const std::vector<AttributeModification> &changes);

}  // namespace simple_ldapd
