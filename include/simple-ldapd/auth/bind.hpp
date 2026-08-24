/**
 * @file bind.hpp
 * @brief Simple bind authentication stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include <string>

namespace simple_ldapd {

class SimpleBindAuthenticator {
public:
  explicit SimpleBindAuthenticator(Backend &backend);

  ResultCode bind(const std::string &dn, const std::string &password) const;

private:
  Backend &backend_;
};

}  // namespace simple_ldapd
