/**
 * @file bind.cpp
 * @brief Simple bind authentication
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/bind.hpp"

#include "simple-ldapd/utils/dn.hpp"

namespace simple_ldapd {

namespace {

bool passwordsEqual(const std::string &left, const std::string &right) {
  const size_t size = left.size() > right.size() ? left.size() : right.size();
  unsigned char acc = static_cast<unsigned char>(left.size() ^ right.size());
  for (size_t i = 0; i < size; ++i) {
    const unsigned char a = i < left.size() ? static_cast<unsigned char>(left[i]) : 0;
    const unsigned char b = i < right.size() ? static_cast<unsigned char>(right[i]) : 0;
    acc = static_cast<unsigned char>(acc | (a ^ b));
  }
  return acc == 0;
}

std::string storedPassword(const std::string &value) {
  constexpr char kCleartext[] = "{CLEARTEXT}";
  const size_t prefix_len = sizeof(kCleartext) - 1;
  if (value.size() >= prefix_len && iequals(value.substr(0, prefix_len), kCleartext)) {
    return value.substr(prefix_len);
  }
  return value;
}

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
  if (!config_.root_dn.empty() && dnEquals(dn, config_.root_dn)) {
    if (config_.root_password.empty() ||
        !passwordsEqual(password, config_.root_password)) {
      return ResultCode::InvalidCredentials;
    }
    return ResultCode::Success;
  }
  const auto entry = backend_.lookup(dn);
  if (!entry) {
    return ResultCode::InvalidCredentials;
  }
  const auto *values = findAttribute(*entry, "userPassword");
  if (values == nullptr || values->empty()) {
    return ResultCode::InvalidCredentials;
  }
  for (const auto &value : *values) {
    if (passwordsEqual(password, storedPassword(value))) {
      return ResultCode::Success;
    }
  }
  return ResultCode::InvalidCredentials;
}

}  // namespace simple_ldapd
