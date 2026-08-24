/**
 * @file rate_limiter.hpp
 * @brief Connection rate limiter stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace simple_ldapd {

class RateLimiter {
public:
  explicit RateLimiter(std::uint32_t max_per_minute = 60);
  bool allow(const std::string &client_id);

private:
  std::uint32_t max_per_minute_;
};

}  // namespace simple_ldapd
