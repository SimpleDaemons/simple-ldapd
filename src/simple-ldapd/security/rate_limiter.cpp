/**
 * @file rate_limiter.cpp
 * @brief Rate limiter stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/rate_limiter.hpp"

namespace simple_ldapd {

RateLimiter::RateLimiter(std::uint32_t max_per_minute)
    : max_per_minute_(max_per_minute) {}

bool RateLimiter::allow(const std::string & /*client_id*/) {
  return max_per_minute_ > 0;
}

}  // namespace simple_ldapd
