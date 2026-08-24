/**
 * @file platform.cpp
 * @brief Platform name helper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/utils/platform.hpp"

namespace simple_ldapd {

std::string platformName() {
#ifdef SIMPLE_LDAPD_WINDOWS
  return "Windows";
#elif defined(SIMPLE_LDAPD_MACOS)
  return "macOS";
#elif defined(SIMPLE_LDAPD_FREEBSD)
  return "FreeBSD";
#elif defined(SIMPLE_LDAPD_LINUX)
  return "Linux";
#else
  return "Unknown";
#endif
}

}  // namespace simple_ldapd
