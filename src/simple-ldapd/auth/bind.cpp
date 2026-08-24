/**
 * @file bind.cpp
 * @brief Simple bind stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/bind.hpp"

namespace simple_ldapd {

SimpleBindAuthenticator::SimpleBindAuthenticator(Backend &backend)
    : backend_(backend) {}

ResultCode SimpleBindAuthenticator::bind(const std::string &dn,
                                         const std::string & /*password*/) const {
  if (dn.empty()) {
    return ResultCode::Success;  // anonymous bind placeholder
  }
  if (!backend_.lookup(dn)) {
    return ResultCode::InvalidCredentials;
  }
  return ResultCode::UnwillingToPerform;
}

}  // namespace simple_ldapd
