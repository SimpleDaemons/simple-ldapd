/**
 * @file acl.cpp
 * @brief Search and write access control
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/acl.hpp"

#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/utils/dn.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace simple_ldapd {

namespace {

std::string trimCopy(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
              value.end());
  return value;
}

std::string lowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

const std::vector<std::string> *findValues(const DirectoryEntry &entry,
                                           const std::string &name) {
  for (const auto &pair : entry.attributes) {
    if (iequals(pair.first, name)) {
      return &pair.second;
    }
  }
  return nullptr;
}

bool hasValue(const DirectoryEntry &entry, const std::string &name, const std::string &value,
              bool as_dn) {
  const auto *values = findValues(entry, name);
  if (values == nullptr) {
    return false;
  }
  for (const auto &item : *values) {
    if (as_dn ? dnEquals(item, value) : iequals(item, value)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool parseAclLine(const std::string &text, AclRule &out, std::string &error) {
  std::istringstream in(text);
  std::string who;
  std::string perm;
  if (!(in >> who >> perm)) {
    error = "acl requires who and permission";
    return false;
  }
  std::string target;
  std::getline(in, target);
  target = trimCopy(target);
  const std::string who_key = lowerCopy(who);
  if (who_key == "*" || who_key == "anyone") {
    out.who = AclWhoKind::Anyone;
    out.who_value.clear();
  } else if (who_key == "anonymous") {
    out.who = AclWhoKind::Anonymous;
    out.who_value.clear();
  } else if (who_key == "users") {
    out.who = AclWhoKind::Users;
    out.who_value.clear();
  } else if (who_key.rfind("dn:", 0) == 0) {
    out.who = AclWhoKind::Dn;
    out.who_value = who.substr(3);
    if (out.who_value.empty()) {
      error = "acl dn: is empty";
      return false;
    }
  } else if (who_key.rfind("group:", 0) == 0) {
    out.who = AclWhoKind::Group;
    out.who_value = who.substr(6);
    if (out.who_value.empty()) {
      error = "acl group: is empty";
      return false;
    }
  } else {
    error = "acl who must be anonymous, users, *, dn:..., or group:...";
    return false;
  }
  const std::string perm_key = lowerCopy(perm);
  if (perm_key == "search") {
    out.permission = AclPermission::Search;
  } else if (perm_key == "write" || perm_key == "*") {
    out.permission = AclPermission::Write;
  } else {
    error = "acl permission must be search or write";
    return false;
  }
  if (target.empty() || target == "*") {
    out.target.clear();
  } else {
    out.target = target;
  }
  error.clear();
  return true;
}

AccessControl::AccessControl(const LdapConfig &config) : config_(config) {}

bool AccessControl::configured() const { return !config_.acls.empty(); }

bool AccessControl::isRoot(const std::string &bind_dn) const {
  return !bind_dn.empty() && !config_.root_dn.empty() && dnEquals(bind_dn, config_.root_dn);
}

bool AccessControl::covers(const AclRule &rule, const std::string &entry_dn) const {
  if (rule.target.empty()) {
    return true;
  }
  return dnEquals(entry_dn, rule.target) || dnEndsWith(entry_dn, rule.target);
}

bool AccessControl::grants(const AclRule &rule, bool write) const {
  if (write) {
    return rule.permission == AclPermission::Write;
  }
  return true;
}

bool AccessControl::inGroup(const std::string &bind_dn, const std::string &group_dn,
                            Backend &backend) const {
  if (bind_dn.empty()) {
    return false;
  }
  const auto group = backend.lookup(group_dn);
  if (!group) {
    return false;
  }
  if (hasValue(*group, "member", bind_dn, true) ||
      hasValue(*group, "uniqueMember", bind_dn, true)) {
    return true;
  }
  const auto user = backend.lookup(bind_dn);
  if (user && hasValue(*user, "memberOf", group_dn, true)) {
    return true;
  }
  std::string uid;
  if (user) {
    const auto *values = findValues(*user, "uid");
    if (values != nullptr && !values->empty()) {
      uid = values->front();
    }
  }
  if (uid.empty()) {
    std::string type;
    std::string value;
    if (parseRdn(dnRdn(bind_dn), type, value) && iequals(type, "uid")) {
      uid = value;
    }
  }
  return !uid.empty() && hasValue(*group, "memberUid", uid, false);
}

bool AccessControl::whoMatches(const AclRule &rule, const std::string &bind_dn,
                               Backend &backend) const {
  switch (rule.who) {
  case AclWhoKind::Anyone:
    return true;
  case AclWhoKind::Anonymous:
    return bind_dn.empty();
  case AclWhoKind::Users:
    return !bind_dn.empty();
  case AclWhoKind::Dn:
    return dnEquals(bind_dn, rule.who_value);
  case AclWhoKind::Group:
    return inGroup(bind_dn, rule.who_value, backend);
  }
  return false;
}

bool AccessControl::maySearch(const std::string &bind_dn, const std::string &entry_dn,
                              Backend &backend) const {
  if (entry_dn.empty() || isRoot(bind_dn)) {
    return true;
  }
  if (config_.acls.empty()) {
    return true;
  }
  for (const auto &rule : config_.acls) {
    if (whoMatches(rule, bind_dn, backend) && covers(rule, entry_dn) &&
        grants(rule, false)) {
      return true;
    }
  }
  return false;
}

bool AccessControl::mayWrite(const std::string &bind_dn, const std::string &entry_dn,
                             Backend &backend) const {
  if (isRoot(bind_dn)) {
    return true;
  }
  if (config_.acls.empty()) {
    return false;
  }
  for (const auto &rule : config_.acls) {
    if (whoMatches(rule, bind_dn, backend) && covers(rule, entry_dn) &&
        grants(rule, true)) {
      return true;
    }
  }
  return false;
}

}  // namespace simple_ldapd
