/**
 * @file rate_limiter.hpp
 * @brief Per-client bind rate limiter
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace simple_ldapd {

class RateLimiter {
public:
  explicit RateLimiter(std::uint32_t max_per_minute = 0);

  void setMaxPerMinute(std::uint32_t max_per_minute);
  bool allow(const std::string &client_id);
  std::uint32_t maxPerMinute() const { return max_per_minute_; }

private:
  struct Bucket {
    std::chrono::steady_clock::time_point window_start{};
    std::uint32_t count{0};
  };

  static std::string clientKey(const std::string &client_id);
  void pruneLocked(std::chrono::steady_clock::time_point now);

  std::uint32_t max_per_minute_{0};
  mutable std::mutex mutex_;
  std::map<std::string, Bucket> buckets_;
};

}  // namespace simple_ldapd
