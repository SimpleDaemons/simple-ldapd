/**
 * @file ber.cpp
 * @brief BER codec stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/protocol/ber.hpp"

namespace simple_ldapd {

bool BerCodec::decode(const std::vector<uint8_t> & /*wire*/) { return false; }

std::vector<uint8_t> BerCodec::encode() const { return {}; }

}  // namespace simple_ldapd
