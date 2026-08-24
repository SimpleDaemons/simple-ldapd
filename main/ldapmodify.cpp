/**
 * @file ldapmodify.cpp
 * @brief Modify entries from LDIF
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
    printClientUsage("ldapmodify", "Modify entries from LDIF");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapmodify", "Modify entries from LDIF");
    return 0;
  }
  if (options.version) {
    printVersion("ldapmodify");
    return 0;
  }
  return notImplementedExit("ldapmodify");
}
