/**
 * @file dn.cpp
 * @brief DN and attribute-name helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/utils/dn.hpp"

#include <algorithm>
#include <cctype>

namespace simple_ldapd {

std::string toLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool iequals(const std::string &left, const std::string &right) {
  return toLowerAscii(left) == toLowerAscii(right);
}

bool dnEquals(const std::string &left, const std::string &right) {
  return iequals(left, right);
}

bool dnEndsWith(const std::string &dn, const std::string &base) {
  if (base.empty()) {
    return true;
  }
  const std::string hay = toLowerAscii(dn);
  const std::string needle = toLowerAscii(base);
  if (hay == needle) {
    return true;
  }
  return hay.size() > needle.size() &&
         hay.compare(hay.size() - needle.size(), needle.size(), needle) == 0 &&
         hay[hay.size() - needle.size() - 1] == ',';
}

bool dnIsOneLevelChild(const std::string &dn, const std::string &base) {
  if (base.empty()) {
    return dn.find(',') == std::string::npos && !dn.empty();
  }
  if (!dnEndsWith(dn, base) || dnEquals(dn, base)) {
    return false;
  }
  const std::string hay = toLowerAscii(dn);
  const std::string needle = toLowerAscii(base);
  const std::string rdn = hay.substr(0, hay.size() - needle.size() - 1);
  return rdn.find(',') == std::string::npos;
}

}  // namespace simple_ldapd
