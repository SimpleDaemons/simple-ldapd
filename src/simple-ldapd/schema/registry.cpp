/**
 * @file registry.cpp
 * @brief Schema file loader
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/schema/registry.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

namespace simple_ldapd {

namespace {

std::string extractName(const std::string &line) {
  auto pos = line.find("NAME");
  if (pos == std::string::npos) {
    return {};
  }
  auto quote = line.find('\'', pos);
  if (quote == std::string::npos) {
    return {};
  }
  auto end = line.find('\'', quote + 1);
  if (end == std::string::npos) {
    return {};
  }
  return line.substr(quote + 1, end - quote - 1);
}

std::string extractOid(const std::string &line) {
  auto start = line.find('(');
  if (start == std::string::npos) {
    return {};
  }
  start += 1;
  while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
    ++start;
  }
  auto end = start;
  while (end < line.size() &&
         (std::isdigit(static_cast<unsigned char>(line[end])) || line[end] == '.')) {
    ++end;
  }
  return line.substr(start, end - start);
}

}  // namespace

bool SchemaRegistry::loadFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    if (line.find("attributetype") != std::string::npos ||
        line.find("attributeType") != std::string::npos) {
      SchemaAttribute attr;
      attr.oid = extractOid(line);
      attr.name = extractName(line);
      if (!attr.name.empty()) {
        attributes_.push_back(attr);
      }
    } else if (line.find("objectclass") != std::string::npos ||
               line.find("objectClass") != std::string::npos) {
      SchemaObjectClass oc;
      oc.oid = extractOid(line);
      oc.name = extractName(line);
      if (!oc.name.empty()) {
        object_classes_.push_back(oc);
      }
    }
  }
  return true;
}

bool SchemaRegistry::loadDirectory(const std::string &directory) {
#ifdef _WIN32
  std::string pattern = directory + "\\*.schema";
  WIN32_FIND_DATAA data;
  HANDLE handle = FindFirstFileA(pattern.c_str(), &data);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  bool ok = true;
  do {
    std::string path = directory + "\\" + data.cFileName;
    ok = loadFile(path) && ok;
  } while (FindNextFileA(handle, &data));
  FindClose(handle);
  return ok;
#else
  DIR *dir = opendir(directory.c_str());
  if (dir == nullptr) {
    return false;
  }
  bool ok = true;
  while (dirent *entry = readdir(dir)) {
    std::string name = entry->d_name;
    if (name.size() > 7 && name.compare(name.size() - 7, 7, ".schema") == 0) {
      ok = loadFile(directory + "/" + name) && ok;
    }
  }
  closedir(dir);
  return ok;
#endif
}

bool SchemaRegistry::hasAttribute(const std::string &name) const {
  return std::any_of(attributes_.begin(), attributes_.end(),
                     [&](const SchemaAttribute &attr) { return attr.name == name; });
}

bool SchemaRegistry::hasObjectClass(const std::string &name) const {
  return std::any_of(object_classes_.begin(), object_classes_.end(),
                     [&](const SchemaObjectClass &oc) { return oc.name == name; });
}

}  // namespace simple_ldapd
