/**
 * @file acl.hpp
 * @brief Search and write access control
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include <string>

namespace simple_ldapd {

class LdapConfig;

enum class AclWhoKind { Anyone, Anonymous, Users, Dn, Group };

enum class AclPermission { Search, Write };

struct AclRule {
  AclWhoKind who{AclWhoKind::Anonymous};
  std::string who_value;
  AclPermission permission{AclPermission::Search};
  std::string target;
};

bool parseAclLine(const std::string &text, AclRule &out, std::string &error);

class AccessControl {
public:
  explicit AccessControl(const LdapConfig &config);

  bool configured() const;
  bool maySearch(const std::string &bind_dn, const std::string &entry_dn,
                 Backend &backend) const;
  bool mayWrite(const std::string &bind_dn, const std::string &entry_dn,
                Backend &backend) const;

private:
  const LdapConfig &config_;

  bool isRoot(const std::string &bind_dn) const;
  bool whoMatches(const AclRule &rule, const std::string &bind_dn, Backend &backend) const;
  bool covers(const AclRule &rule, const std::string &entry_dn) const;
  bool grants(const AclRule &rule, bool write) const;
  bool inGroup(const std::string &bind_dn, const std::string &group_dn,
               Backend &backend) const;
};

}  // namespace simple_ldapd
