/**
 * @file sqlite.hpp
 * @brief SQLite directory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include <memory>
#include <string>

namespace simple_ldapd {

class SqliteBackend : public Backend {
public:
  explicit SqliteBackend(std::string db_path, std::string ldif_seed = {});
  ~SqliteBackend() override;

  SqliteBackend(const SqliteBackend &) = delete;
  SqliteBackend &operator=(const SqliteBackend &) = delete;
  SqliteBackend(SqliteBackend &&) = delete;
  SqliteBackend &operator=(SqliteBackend &&) = delete;

  bool initialize() override;
  std::string name() const override { return "sqlite"; }

  std::optional<DirectoryEntry> lookup(const std::string &dn) const override;
  std::vector<DirectoryEntry> search(const std::string &base_dn, SearchScope scope,
                                     const SearchFilter &filter) const override;
  bool add(const DirectoryEntry &entry) override;
  bool modify(const DirectoryEntry &entry) override;
  bool remove(const std::string &dn) override;
  bool rename(const std::string &from, const std::string &to) override;
  bool hasChildren(const std::string &dn) const override;
  void persist() override;

  bool exportFile(const std::string &path) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace simple_ldapd
