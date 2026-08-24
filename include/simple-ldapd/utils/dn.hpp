/**
 * @file dn.hpp
 * @brief DN and attribute-name helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <string>

namespace simple_ldapd {

std::string toLowerAscii(std::string value);
bool iequals(const std::string &left, const std::string &right);
bool dnEquals(const std::string &left, const std::string &right);
bool dnEndsWith(const std::string &dn, const std::string &base);
bool dnIsOneLevelChild(const std::string &dn, const std::string &base);

}  // namespace simple_ldapd
