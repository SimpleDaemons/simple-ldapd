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
std::string dnParent(const std::string &dn);
std::string dnRdn(const std::string &dn);
std::string composeDn(const std::string &rdn, const std::string &parent);
bool parseRdn(const std::string &rdn, std::string &type, std::string &value);

}  // namespace simple_ldapd
