/**
 * @file ldif.cpp
 * @brief LDIF content and change-record parsing
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/protocol/ldif.hpp"

#include "simple-ldapd/utils/dn.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

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

LdifChangeType parseChangeType(const std::string &value) {
  if (iequals(value, "add")) {
    return LdifChangeType::Add;
  }
  if (iequals(value, "modify")) {
    return LdifChangeType::Modify;
  }
  if (iequals(value, "delete")) {
    return LdifChangeType::Delete;
  }
  if (iequals(value, "modrdn") || iequals(value, "moddn")) {
    return LdifChangeType::Modrdn;
  }
  return LdifChangeType::Content;
}

ModifyOp parseModifyOp(const std::string &value) {
  if (iequals(value, "add")) {
    return ModifyOp::Add;
  }
  if (iequals(value, "delete")) {
    return ModifyOp::Delete;
  }
  return ModifyOp::Replace;
}

void flushChange(LdifRecord &record, AttributeModification &current, bool &have_change) {
  if (have_change) {
    record.changes.push_back(current);
    current = AttributeModification{};
    have_change = false;
  }
}

}  // namespace

std::vector<LdifRecord> parseLdifText(const std::string &text) {
  std::vector<LdifRecord> records;
  LdifRecord current;
  AttributeModification change;
  bool have_change = false;
  std::istringstream in(text);
  std::string line;
  auto flush = [&]() {
    flushChange(current, change, have_change);
    if (!current.dn.empty()) {
      current.entry.dn = current.dn;
      records.push_back(current);
      current = LdifRecord{};
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
    if (iequals(line.substr(0, std::min<size_t>(line.size(), 8)), "version:")) {
      continue;
    }
    auto colon = line.find(':');
    if (colon == std::string::npos) {
      if (line == "-") {
        flushChange(current, change, have_change);
      }
      continue;
    }
    std::string name = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    trimInPlace(name);
    trimInPlace(value);
    if (iequals(name, "dn")) {
      flush();
      current.dn = value;
    } else if (iequals(name, "changetype")) {
      current.changetype = parseChangeType(value);
    } else if (iequals(name, "newrdn")) {
      current.new_rdn = value;
    } else if (iequals(name, "deleteoldrdn")) {
      current.delete_old_rdn = value == "1" || iequals(value, "true");
    } else if (iequals(name, "newsuperior")) {
      current.new_superior = value;
    } else if (iequals(name, "add") || iequals(name, "delete") || iequals(name, "replace")) {
      flushChange(current, change, have_change);
      change.op = parseModifyOp(name);
      change.type = value;
      have_change = true;
    } else {
      current.entry.attributes[name].push_back(value);
      if (have_change && iequals(change.type, name)) {
        change.values.push_back(value);
      }
    }
  }
  flush();
  return records;
}

std::vector<LdifRecord> parseLdifFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return {};
  }
  std::ostringstream text;
  text << in.rdbuf();
  return parseLdifText(text.str());
}

std::string formatLdif(const std::vector<DirectoryEntry> &entries) {
  std::ostringstream out;
  for (const auto &entry : entries) {
    out << "dn: " << entry.dn << "\n";
    for (const auto &pair : entry.attributes) {
      for (const auto &value : pair.second) {
        out << pair.first << ": " << value << "\n";
      }
    }
    out << "\n";
  }
  return out.str();
}

}  // namespace simple_ldapd
