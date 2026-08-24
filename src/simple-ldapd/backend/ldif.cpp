/**
 * @file ldif.cpp
 * @brief LDIF backend stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/ldif.hpp"

#include "simple-ldapd/utils/logger.hpp"
#include <fstream>

namespace simple_ldapd {

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
    if (line.empty()) {
      flush();
      continue;
    }
    if (line.rfind("dn:", 0) == 0) {
      flush();
      current.dn = line.substr(3);
      while (!current.dn.empty() && current.dn.front() == ' ') {
        current.dn.erase(current.dn.begin());
      }
    } else {
      auto colon = line.find(':');
      if (colon == std::string::npos) {
        continue;
      }
      std::string name = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      while (!value.empty() && value.front() == ' ') {
        value.erase(value.begin());
      }
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
  out << "# LDIF export is a skeleton stub\n";
  return static_cast<bool>(out);
}

}  // namespace simple_ldapd
