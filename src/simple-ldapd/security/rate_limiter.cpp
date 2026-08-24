/**
 * @file rate_limiter.cpp
 * @brief Per-client bind rate limiter
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/rate_limiter.hpp"

namespace simple_ldapd {

namespace {

constexpr auto kWindow = std::chrono::minutes(1);

}  // namespace

RateLimiter::RateLimiter(std::uint32_t max_per_minute) : max_per_minute_(max_per_minute) {}

void RateLimiter::setMaxPerMinute(std::uint32_t max_per_minute) {
  std::lock_guard<std::mutex> lock(mutex_);
  max_per_minute_ = max_per_minute;
  buckets_.clear();
}

std::string RateLimiter::clientKey(const std::string &client_id) {
  if (!client_id.empty() && client_id.front() == '[') {
    const auto close = client_id.find(']');
    if (close != std::string::npos) {
      return client_id.substr(0, close + 1);
    }
  }
  const auto colon = client_id.rfind(':');
  if (colon != std::string::npos && client_id.find(':') == colon) {
    return client_id.substr(0, colon);
  }
  return client_id;
}

void RateLimiter::pruneLocked(std::chrono::steady_clock::time_point now) {
  if (buckets_.size() < 4096) {
    return;
  }
  for (auto it = buckets_.begin(); it != buckets_.end();) {
    if (now - it->second.window_start >= kWindow) {
      it = buckets_.erase(it);
    } else {
      ++it;
    }
  }
}

bool RateLimiter::allow(const std::string &client_id) {
  if (max_per_minute_ == 0) {
    return true;
  }
  const std::string key = clientKey(client_id);
  const auto now = std::chrono::steady_clock::now();
  std::lock_guard<std::mutex> lock(mutex_);
  pruneLocked(now);
  auto &bucket = buckets_[key];
  if (bucket.window_start.time_since_epoch().count() == 0 || now - bucket.window_start >= kWindow) {
    bucket.window_start = now;
    bucket.count = 0;
  }
  if (bucket.count >= max_per_minute_) {
    return false;
  }
  ++bucket.count;
  return true;
}

}  // namespace simple_ldapd
