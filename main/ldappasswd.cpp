/**
 * @file ldappasswd.cpp
 * @brief Change a user password
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/common.hpp"

int main(int argc, char *argv[]) {
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::cli::notImplementedExit;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldappasswd", "Change a user password");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldappasswd", "Change a user password");
    return 0;
  }
  if (options.version) {
    printVersion("ldappasswd");
    return 0;
  }
  return notImplementedExit("ldappasswd");
}
