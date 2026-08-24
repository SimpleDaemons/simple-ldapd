/**
 * @file common.hpp
 * @brief Shared OpenLDAP-style CLI helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

namespace simple_ldapd {
namespace cli {

struct ClientOptions {
  std::string uri{"ldap://127.0.0.1:389"};
  std::string bind_dn;
  std::string password;
  std::string base_dn;
  std::string filter{"(objectClass=*)"};
  std::string ldif_file;
  bool simple_auth{true};
  bool prompt_password{false};
  bool help{false};
  bool version{false};
  bool parse_error{false};
};

void printClientUsage(const std::string &tool, const std::string &summary);
void printVersion(const std::string &tool);
ClientOptions parseClientArgs(int argc, char *argv[]);
int notImplementedExit(const std::string &tool);

}  // namespace cli
}  // namespace simple_ldapd
