/**
 * @file session.hpp
 * @brief LDAP session state
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/core/connection.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include <string>

namespace simple_ldapd {

class Session {
public:
  explicit Session(Connection connection);

  bool bound() const { return bound_; }
  const std::string &bindDn() const { return bind_dn_; }
  LdapMessage handle(const LdapMessage &request);

private:
  Connection connection_;
  bool bound_{false};
  std::string bind_dn_;
};

}  // namespace simple_ldapd
