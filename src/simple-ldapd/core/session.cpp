/**
 * @file session.cpp
 * @brief LDAP session stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/core/session.hpp"

namespace simple_ldapd {

Session::Session(Connection connection) : connection_(std::move(connection)) {}

LdapMessage Session::handle(const LdapMessage &request) {
  LdapMessage response = request;
  response.result = ResultCode::Unavailable;
  response.diagnostic = "LDAPv3 operations are not implemented yet";
  return response;
}

}  // namespace simple_ldapd
