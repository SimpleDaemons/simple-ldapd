/**
 * @file bind.hpp
 * @brief Simple bind authentication
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include <optional>
#include <string>

namespace simple_ldapd {

class SimpleBindAuthenticator {
public:
  SimpleBindAuthenticator(Backend &backend, const LdapConfig &config);

  ResultCode bind(const std::string &dn, const std::string &password) const;
  std::optional<std::string> resolveName(const std::string &name) const;
  std::optional<std::string> passwordFor(const std::string &dn) const;
  bool isAccountDisabled(const std::string &dn) const;

private:
  std::optional<std::string> findAccount(const std::string &name) const;

private:
  Backend &backend_;
  const LdapConfig &config_;
};

}  // namespace simple_ldapd
