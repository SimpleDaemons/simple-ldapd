/**
 * @file filter.hpp
 * @brief LDAP search filter stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <string>

namespace simple_ldapd {

class SearchFilter {
public:
  static SearchFilter parse(const std::string &text);
  const std::string &text() const;
  bool valid() const;

private:
  std::string text_;
  bool valid_{false};
};

}  // namespace simple_ldapd
