/**
 * @file ldif.cpp
 * @brief LDIF import into the in-memory backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/ldif.hpp"

#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/utils/logger.hpp"
#include <fstream>

namespace simple_ldapd {

namespace {

void trimInPlace(std::string &value) {
  while (!value.empty() && (value.back() == '\r' || value.back() == ' ' ||
                            value.back() == '\t')) {
    value.pop_back();
  }
  size_t start = 0;
  while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) {
    ++start;
  }
  if (start != 0) {
    value.erase(0, start);
  }
}

}  // namespace

LdifBackend::LdifBackend(std::string path) : path_(std::move(path)) {}

bool LdifBackend::initialize() {
  if (!MemoryBackend::initialize()) {
    return false;
  }
  if (path_.empty()) {
    return true;
  }
  return importFile(path_);
}

bool LdifBackend::importFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    Logger::instance().warning("LDIF import not available: " + path);
    return false;
  }
  DirectoryEntry current;
  std::string line;
  auto flush = [this, &current]() {
    if (!current.dn.empty()) {
      add(current);
      current = DirectoryEntry{};
    }
  };
  while (std::getline(in, line)) {
    trimInPlace(line);
    if (line.empty()) {
      flush();
      continue;
    }
    if (line.front() == '#') {
      continue;
    }
    if (line.rfind("dn:", 0) == 0) {
      flush();
      current.dn = line.substr(3);
      trimInPlace(current.dn);
    } else {
      auto colon = line.find(':');
      if (colon == std::string::npos) {
        continue;
      }
      std::string name = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      trimInPlace(name);
      trimInPlace(value);
      current.attributes[name].push_back(value);
    }
  }
  flush();
  return true;
}

bool LdifBackend::exportFile(const std::string &path) const {
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  const auto entries = search({}, SearchScope::Subtree, SearchFilter::all());
  for (const auto &entry : entries) {
    out << "dn: " << entry.dn << "\n";
    for (const auto &pair : entry.attributes) {
      for (const auto &value : pair.second) {
        out << pair.first << ": " << value << "\n";
      }
    }
    out << "\n";
  }
  return static_cast<bool>(out);
}

void LdifBackend::persist() {
  if (path_.empty()) {
    return;
  }
  if (!exportFile(path_)) {
    Logger::instance().warning("LDIF persist failed: " + path_);
  }
}

}  // namespace simple_ldapd
