/**
 * @file ber.hpp
 * @brief BER codec stub for LDAP messages
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

namespace simple_ldapd {

class BerCodec {
public:
  bool decode(const std::vector<uint8_t> &wire);
  std::vector<uint8_t> encode() const;
  bool implemented() const { return false; }
};

}  // namespace simple_ldapd
