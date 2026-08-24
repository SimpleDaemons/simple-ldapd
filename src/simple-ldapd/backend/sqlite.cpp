/**
 * @file sqlite.cpp
 * @brief SQLite directory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/sqlite.hpp"

#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/ldif.hpp"
#include "simple-ldapd/utils/dn.hpp"
#include "simple-ldapd/utils/logger.hpp"
#include <fstream>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#ifdef SIMPLE_LDAPD_SQLITE
#include <sqlite3.h>
#endif

namespace simple_ldapd {

#ifdef SIMPLE_LDAPD_SQLITE

namespace {

struct SqliteCloser {
  void operator()(sqlite3 *db) const {
    if (db != nullptr) {
      sqlite3_close(db);
    }
  }
};

struct SqliteStmtCloser {
  void operator()(sqlite3_stmt *stmt) const {
    if (stmt != nullptr) {
      sqlite3_finalize(stmt);
    }
  }
};

using SqliteHandle = std::unique_ptr<sqlite3, SqliteCloser>;
using SqliteStmt = std::unique_ptr<sqlite3_stmt, SqliteStmtCloser>;

SqliteStmt prepare(sqlite3 *db, const char *sql) {
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return {};
  }
  return SqliteStmt(stmt);
}

bool bindText(sqlite3_stmt *stmt, int index, const std::string &value) {
  return sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()),
                           SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt *stmt, int index) {
  const unsigned char *text = sqlite3_column_text(stmt, index);
  if (text == nullptr) {
    return {};
  }
  return reinterpret_cast<const char *>(text);
}

}  // namespace

struct SqliteBackend::Impl {
  std::string db_path;
  std::string ldif_seed;
  SqliteHandle db;
  mutable std::mutex mutex;

  bool exec(const char *sql) {
    char *err = nullptr;
    const int rc = sqlite3_exec(db.get(), sql, nullptr, nullptr, &err);
    if (err != nullptr) {
      Logger::instance().warning(std::string("sqlite: ") + err);
      sqlite3_free(err);
    }
    return rc == SQLITE_OK;
  }

  bool begin() { return exec("BEGIN IMMEDIATE"); }
  bool commit() { return exec("COMMIT"); }
  void rollback() { exec("ROLLBACK"); }

  std::optional<std::string> storedDn(const std::string &dn) const {
    auto stmt = prepare(db.get(), "SELECT dn FROM entries WHERE lower(dn) = lower(?1)");
    if (!stmt || !bindText(stmt.get(), 1, dn)) {
      return std::nullopt;
    }
    if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
      return std::nullopt;
    }
    return columnText(stmt.get(), 0);
  }

  DirectoryEntry loadAttributes(const std::string &dn) const {
    DirectoryEntry entry;
    entry.dn = dn;
    auto stmt = prepare(db.get(),
                        "SELECT type, value FROM attributes WHERE dn = ?1 ORDER BY ord");
    if (!stmt || !bindText(stmt.get(), 1, dn)) {
      return entry;
    }
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
      entry.attributes[columnText(stmt.get(), 0)].push_back(columnText(stmt.get(), 1));
    }
    return entry;
  }

  bool writeAttributes(const DirectoryEntry &entry) {
    auto del = prepare(db.get(), "DELETE FROM attributes WHERE dn = ?1");
    if (!del || !bindText(del.get(), 1, entry.dn) || sqlite3_step(del.get()) != SQLITE_DONE) {
      return false;
    }
    auto ins =
        prepare(db.get(), "INSERT INTO attributes(dn, type, value, ord) VALUES(?1, ?2, ?3, ?4)");
    if (!ins) {
      return false;
    }
    int ord = 0;
    for (const auto &pair : entry.attributes) {
      for (const auto &value : pair.second) {
        sqlite3_reset(ins.get());
        sqlite3_clear_bindings(ins.get());
        if (!bindText(ins.get(), 1, entry.dn) || !bindText(ins.get(), 2, pair.first) ||
            !bindText(ins.get(), 3, value) ||
            sqlite3_bind_int(ins.get(), 4, ord++) != SQLITE_OK ||
            sqlite3_step(ins.get()) != SQLITE_DONE) {
          return false;
        }
      }
    }
    return true;
  }

  int entryCount() const {
    auto stmt = prepare(db.get(), "SELECT COUNT(*) FROM entries");
    if (!stmt || sqlite3_step(stmt.get()) != SQLITE_ROW) {
      return -1;
    }
    return sqlite3_column_int(stmt.get(), 0);
  }

  std::vector<DirectoryEntry> allEntries() const {
    std::map<std::string, DirectoryEntry> by_dn;
    auto dns = prepare(db.get(), "SELECT dn FROM entries");
    if (dns) {
      while (sqlite3_step(dns.get()) == SQLITE_ROW) {
        const std::string dn = columnText(dns.get(), 0);
        DirectoryEntry entry;
        entry.dn = dn;
        by_dn[dn] = std::move(entry);
      }
    }
    auto attrs =
        prepare(db.get(), "SELECT dn, type, value FROM attributes ORDER BY dn, ord");
    if (attrs) {
      while (sqlite3_step(attrs.get()) == SQLITE_ROW) {
        const std::string dn = columnText(attrs.get(), 0);
        auto it = by_dn.find(dn);
        if (it != by_dn.end()) {
          it->second.attributes[columnText(attrs.get(), 1)].push_back(columnText(attrs.get(), 2));
        }
      }
    }
    std::vector<DirectoryEntry> entries;
    entries.reserve(by_dn.size());
    for (auto &pair : by_dn) {
      entries.push_back(std::move(pair.second));
    }
    return entries;
  }
};

SqliteBackend::SqliteBackend(std::string db_path, std::string ldif_seed)
    : impl_(std::make_unique<Impl>()) {
  impl_->db_path = std::move(db_path);
  impl_->ldif_seed = std::move(ldif_seed);
}

SqliteBackend::~SqliteBackend() = default;

bool SqliteBackend::initialize() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (impl_->db_path.empty()) {
    Logger::instance().error("sqlite backend requires sqlite_file");
    return false;
  }
  sqlite3 *raw = nullptr;
  if (sqlite3_open(impl_->db_path.c_str(), &raw) != SQLITE_OK) {
    Logger::instance().error("sqlite open failed: " + impl_->db_path);
    if (raw != nullptr) {
      sqlite3_close(raw);
    }
    return false;
  }
  impl_->db.reset(raw);
  sqlite3_busy_timeout(impl_->db.get(), 5000);
  if (!impl_->exec("PRAGMA foreign_keys = ON") || !impl_->exec("PRAGMA journal_mode = WAL") ||
      !impl_->exec("CREATE TABLE IF NOT EXISTS entries (dn TEXT PRIMARY KEY NOT NULL)") ||
      !impl_->exec("CREATE TABLE IF NOT EXISTS attributes ("
                   "dn TEXT NOT NULL, type TEXT NOT NULL, value TEXT NOT NULL, ord INTEGER NOT NULL, "
                   "FOREIGN KEY(dn) REFERENCES entries(dn) ON DELETE CASCADE ON UPDATE CASCADE)") ||
      !impl_->exec("CREATE INDEX IF NOT EXISTS attributes_dn ON attributes(dn)")) {
    return false;
  }
  if (impl_->entryCount() == 0 && !impl_->ldif_seed.empty()) {
    if (!impl_->begin()) {
      return false;
    }
    for (const auto &record : parseLdifFile(impl_->ldif_seed)) {
      if (record.dn.empty() || record.entry.dn.empty()) {
        continue;
      }
      if (record.changetype != LdifChangeType::Content &&
          record.changetype != LdifChangeType::Add) {
        continue;
      }
      auto ins = prepare(impl_->db.get(), "INSERT INTO entries(dn) VALUES(?1)");
      if (!ins || !bindText(ins.get(), 1, record.entry.dn) ||
          sqlite3_step(ins.get()) != SQLITE_DONE || !impl_->writeAttributes(record.entry)) {
        impl_->rollback();
        Logger::instance().warning("sqlite LDIF seed failed for " + record.entry.dn);
        return false;
      }
    }
    if (!impl_->commit()) {
      impl_->rollback();
      return false;
    }
  }
  return true;
}

std::optional<DirectoryEntry> SqliteBackend::lookup(const std::string &dn) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db) {
    return std::nullopt;
  }
  const auto stored = impl_->storedDn(dn);
  if (!stored) {
    return std::nullopt;
  }
  return impl_->loadAttributes(*stored);
}

std::vector<DirectoryEntry> SqliteBackend::search(const std::string &base_dn, SearchScope scope,
                                                  const SearchFilter &filter) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  std::vector<DirectoryEntry> matches;
  if (!impl_->db) {
    return matches;
  }
  for (const auto &entry : impl_->allEntries()) {
    bool in_scope = false;
    switch (scope) {
    case SearchScope::Base:
      in_scope = dnEquals(entry.dn, base_dn);
      break;
    case SearchScope::OneLevel:
      in_scope = dnIsOneLevelChild(entry.dn, base_dn);
      break;
    case SearchScope::Subtree:
      in_scope = dnEndsWith(entry.dn, base_dn);
      break;
    }
    if (in_scope && filter.matches(entry)) {
      matches.push_back(entry);
    }
  }
  return matches;
}

bool SqliteBackend::add(const DirectoryEntry &entry) {
  if (entry.dn.empty()) {
    return false;
  }
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db || impl_->storedDn(entry.dn)) {
    return false;
  }
  if (!impl_->begin()) {
    return false;
  }
  auto ins = prepare(impl_->db.get(), "INSERT INTO entries(dn) VALUES(?1)");
  if (!ins || !bindText(ins.get(), 1, entry.dn) || sqlite3_step(ins.get()) != SQLITE_DONE ||
      !impl_->writeAttributes(entry) || !impl_->commit()) {
    impl_->rollback();
    return false;
  }
  return true;
}

bool SqliteBackend::modify(const DirectoryEntry &entry) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db) {
    return false;
  }
  const auto stored = impl_->storedDn(entry.dn);
  if (!stored) {
    return false;
  }
  DirectoryEntry copy = entry;
  copy.dn = *stored;
  if (!impl_->begin() || !impl_->writeAttributes(copy) || !impl_->commit()) {
    impl_->rollback();
    return false;
  }
  return true;
}

bool SqliteBackend::remove(const std::string &dn) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db) {
    return false;
  }
  const auto stored = impl_->storedDn(dn);
  if (!stored) {
    return false;
  }
  if (!impl_->begin()) {
    return false;
  }
  auto stmt = prepare(impl_->db.get(), "DELETE FROM entries WHERE dn = ?1");
  if (!stmt || !bindText(stmt.get(), 1, *stored) || sqlite3_step(stmt.get()) != SQLITE_DONE ||
      !impl_->commit()) {
    impl_->rollback();
    return false;
  }
  return true;
}

bool SqliteBackend::rename(const std::string &from, const std::string &to) {
  if (from.empty() || to.empty()) {
    return false;
  }
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db) {
    return false;
  }
  const auto stored = impl_->storedDn(from);
  if (!stored) {
    return false;
  }
  if (dnEquals(from, to)) {
    return true;
  }
  if (impl_->storedDn(to)) {
    return false;
  }
  if (!impl_->begin()) {
    return false;
  }
  auto stmt = prepare(impl_->db.get(), "UPDATE entries SET dn = ?1 WHERE dn = ?2");
  if (!stmt || !bindText(stmt.get(), 1, to) || !bindText(stmt.get(), 2, *stored) ||
      sqlite3_step(stmt.get()) != SQLITE_DONE || !impl_->commit()) {
    impl_->rollback();
    return false;
  }
  return true;
}

bool SqliteBackend::hasChildren(const std::string &dn) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db) {
    return false;
  }
  for (const auto &entry : impl_->allEntries()) {
    if (dnIsOneLevelChild(entry.dn, dn)) {
      return true;
    }
  }
  return false;
}

void SqliteBackend::persist() {}

bool SqliteBackend::exportFile(const std::string &path) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->db) {
    return false;
  }
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << formatLdif(impl_->allEntries());
  return static_cast<bool>(out);
}

#else

struct SqliteBackend::Impl {};

SqliteBackend::SqliteBackend(std::string, std::string) : impl_(std::make_unique<Impl>()) {}

SqliteBackend::~SqliteBackend() = default;

bool SqliteBackend::initialize() {
  Logger::instance().error("sqlite backend was not built (SQLite3 not found)");
  return false;
}

std::optional<DirectoryEntry> SqliteBackend::lookup(const std::string &) const {
  return std::nullopt;
}

std::vector<DirectoryEntry> SqliteBackend::search(const std::string &, SearchScope,
                                                  const SearchFilter &) const {
  return {};
}

bool SqliteBackend::add(const DirectoryEntry &) { return false; }
bool SqliteBackend::modify(const DirectoryEntry &) { return false; }
bool SqliteBackend::remove(const std::string &) { return false; }
bool SqliteBackend::rename(const std::string &, const std::string &) { return false; }
bool SqliteBackend::hasChildren(const std::string &) const { return false; }
void SqliteBackend::persist() {}
bool SqliteBackend::exportFile(const std::string &) const { return false; }

#endif

}  // namespace simple_ldapd
