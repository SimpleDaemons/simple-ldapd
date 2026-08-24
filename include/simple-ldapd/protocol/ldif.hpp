/**
 * @file ldif.hpp
 * @brief LDIF content and change-record parsing
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/protocol/message.hpp"
#include <string>
#include <vector>

namespace simple_ldapd {

enum class LdifChangeType { Content, Add, Modify, Delete, Modrdn };

struct LdifRecord {
  std::string dn;
  LdifChangeType changetype{LdifChangeType::Content};
  DirectoryEntry entry;
  std::vector<AttributeModification> changes;
  std::string new_rdn;
  bool delete_old_rdn{true};
  std::string new_superior;
};

std::vector<LdifRecord> parseLdifText(const std::string &text);
std::vector<LdifRecord> parseLdifFile(const std::string &path);
std::string formatLdif(const std::vector<DirectoryEntry> &entries);

}  // namespace simple_ldapd
