/**
 * @file bind.cpp
 * @brief Simple bind authentication
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/bind.hpp"

#include "simple-ldapd/auth/password.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/utils/dn.hpp"

namespace simple_ldapd {

namespace {

const std::vector<std::string> *findAttribute(const DirectoryEntry &entry,
                                              const std::string &name) {
  for (const auto &pair : entry.attributes) {
    if (iequals(pair.first, name)) {
      return &pair.second;
    }
  }
  return nullptr;
}

}  // namespace

SimpleBindAuthenticator::SimpleBindAuthenticator(Backend &backend,
                                                 const LdapConfig &config)
    : backend_(backend), config_(config) {}

ResultCode SimpleBindAuthenticator::bind(const std::string &dn,
                                         const std::string &password) const {
  if (dn.empty()) {
    return password.empty() ? ResultCode::Success : ResultCode::InvalidCredentials;
  }
  const auto resolved = resolveName(dn);
  if (!resolved) {
    return ResultCode::InvalidCredentials;
  }
  if (!config_.root_dn.empty() && dnEquals(*resolved, config_.root_dn)) {
    if (config_.root_password.empty() ||
        !verifyUserPassword(config_.root_password, password)) {
      return ResultCode::InvalidCredentials;
    }
    return ResultCode::Success;
  }
  const auto entry = backend_.lookup(*resolved);
  if (!entry) {
    return ResultCode::InvalidCredentials;
  }
  if (accountDisabled(*entry)) {
    return ResultCode::InvalidCredentials;
  }
  const auto *values = findAttribute(*entry, "userPassword");
  if (values == nullptr || values->empty()) {
    return ResultCode::InvalidCredentials;
  }
  for (const auto &value : *values) {
    if (verifyUserPassword(value, password)) {
      return ResultCode::Success;
    }
  }
  return ResultCode::InvalidCredentials;
}

std::optional<std::string> SimpleBindAuthenticator::findAccount(const std::string &name) const {
  if (name.empty() || name.find_first_of("()*\\") != std::string::npos) {
    return std::nullopt;
  }
  const auto filter =
      SearchFilter::parse("(|(uid=" + name + ")(sAMAccountName=" + name + "))");
  const auto matches = backend_.search(config_.base_dn, SearchScope::Subtree, filter);
  if (matches.size() != 1) {
    return std::nullopt;
  }
  return matches.front().dn;
}

std::optional<std::string> SimpleBindAuthenticator::resolveName(const std::string &name) const {
  if (name.empty()) {
    return std::nullopt;
  }
  if (name.find('=') != std::string::npos) {
    if (!config_.root_dn.empty() && dnEquals(name, config_.root_dn)) {
      return config_.root_dn;
    }
    const auto entry = backend_.lookup(name);
    if (entry) {
      return entry->dn;
    }
    std::string type;
    std::string value;
    if (!parseRdn(dnRdn(name), type, value)) {
      return std::nullopt;
    }
    return findAccount(value);
  }
  if (!config_.root_dn.empty()) {
    std::string type;
    std::string value;
    if (parseRdn(dnRdn(config_.root_dn), type, value) && iequals(value, name)) {
      return config_.root_dn;
    }
  }
  return findAccount(name);
}

std::optional<std::string> SimpleBindAuthenticator::passwordFor(const std::string &dn) const {
  if (!config_.root_dn.empty() && dnEquals(dn, config_.root_dn)) {
    if (config_.root_password.empty()) {
      return std::nullopt;
    }
    return config_.root_password;
  }
  const auto entry = backend_.lookup(dn);
  if (!entry) {
    return std::nullopt;
  }
  const auto *values = findAttribute(*entry, "userPassword");
  if (values == nullptr || values->empty()) {
    return std::nullopt;
  }
  return recoverablePassword(values->front());
}

bool SimpleBindAuthenticator::isAccountDisabled(const std::string &dn) const {
  if (!config_.root_dn.empty() && dnEquals(dn, config_.root_dn)) {
    return false;
  }
  const auto entry = backend_.lookup(dn);
  return entry && accountDisabled(*entry);
}

}  // namespace simple_ldapd
