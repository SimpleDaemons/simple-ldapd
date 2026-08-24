/**
 * @file modify.cpp
 * @brief Attribute modification helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/backend/backend.hpp"

#include "simple-ldapd/utils/dn.hpp"
#include <algorithm>

namespace simple_ldapd {

namespace {

auto findAttribute(DirectoryEntry &entry, const std::string &name) {
  return std::find_if(entry.attributes.begin(), entry.attributes.end(),
                      [&](const auto &pair) { return iequals(pair.first, name); });
}

bool hasValue(const std::vector<std::string> &values, const std::string &wanted) {
  return std::any_of(values.begin(), values.end(),
                     [&](const std::string &value) { return iequals(value, wanted); });
}

}  // namespace

ResultCode applyModifications(DirectoryEntry &entry,
                              const std::vector<AttributeModification> &changes) {
  for (const auto &change : changes) {
    if (change.type.empty()) {
      return ResultCode::InvalidAttributeSyntax;
    }
    auto it = findAttribute(entry, change.type);
    switch (change.op) {
    case ModifyOp::Add:
      if (it == entry.attributes.end()) {
        entry.attributes[change.type] = change.values;
        break;
      }
      for (const auto &value : change.values) {
        if (hasValue(it->second, value)) {
          return ResultCode::AttributeOrValueExists;
        }
        it->second.push_back(value);
      }
      break;
    case ModifyOp::Delete:
      if (it == entry.attributes.end()) {
        return ResultCode::NoSuchAttribute;
      }
      if (change.values.empty()) {
        entry.attributes.erase(it);
        break;
      }
      for (const auto &value : change.values) {
        auto value_it =
            std::find_if(it->second.begin(), it->second.end(),
                         [&](const std::string &existing) { return iequals(existing, value); });
        if (value_it == it->second.end()) {
          return ResultCode::NoSuchAttribute;
        }
        it->second.erase(value_it);
      }
      if (it->second.empty()) {
        entry.attributes.erase(it);
      }
      break;
    case ModifyOp::Replace:
      if (change.values.empty()) {
        if (it != entry.attributes.end()) {
          entry.attributes.erase(it);
        }
      } else if (it == entry.attributes.end()) {
        entry.attributes[change.type] = change.values;
      } else {
        it->second = change.values;
      }
      break;
    }
  }
  return ResultCode::Success;
}

}  // namespace simple_ldapd
