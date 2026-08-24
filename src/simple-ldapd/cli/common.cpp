/**
 * @file common.cpp
 * @brief Shared CLI argument parsing
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/common.hpp"

#include "simple-ldapd/version.hpp"
#include <iostream>

namespace simple_ldapd {
namespace cli {

void printClientUsage(const std::string &tool, const std::string &summary) {
  std::cout << "Usage: " << tool << " [options]\n"
            << summary << "\n\n"
            << "Options:\n"
            << "  -H URI         LDAP URI (ldap:// or ldaps://)\n"
            << "  -h HOST        LDAP host (OpenLDAP-compatible)\n"
            << "  -x             Simple authentication\n"
            << "  -D BIND_DN     Bind DN\n"
            << "  -w PASSWORD    Bind password\n"
            << "  -W             Prompt for bind password\n"
            << "  -b BASE_DN     Search base DN\n"
            << "  -f FILE        LDIF file\n"
            << "  --help         Show this help\n"
            << "  --version      Show version\n";
}

void printVersion(const std::string &tool) {
  std::cout << tool << " (simple-ldapd) " << kVersion << std::endl;
}

ClientOptions parseClientArgs(int argc, char *argv[]) {
  ClientOptions options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        options.parse_error = true;
        return {};
      }
      return argv[++i];
    };
    if (arg == "--help") {
      options.help = true;
    } else if (arg == "--version") {
      options.version = true;
    } else if (arg == "-H") {
      options.uri = next();
    } else if (arg == "-h") {
      options.uri = "ldap://" + next();
    } else if (arg == "-x") {
      options.simple_auth = true;
    } else if (arg == "-D") {
      options.bind_dn = next();
    } else if (arg == "-w") {
      options.password = next();
    } else if (arg == "-W") {
      options.prompt_password = true;
    } else if (arg == "-b") {
      options.base_dn = next();
    } else if (arg == "-f") {
      options.ldif_file = next();
    } else if (!arg.empty() && arg[0] != '-') {
      options.filter = arg;
    }
  }
  return options;
}

int notImplementedExit(const std::string &tool) {
  std::cerr << tool << ": LDAP client operations are not implemented yet"
            << std::endl;
  return 2;
}

}  // namespace cli
}  // namespace simple_ldapd
