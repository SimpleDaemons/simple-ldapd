/**
 * @file common.hpp
 * @brief Shared OpenLDAP-style CLI helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/utils/platform.hpp"
#include <string>
#include <vector>

namespace simple_ldapd {
namespace cli {

struct ClientOptions {
  std::string uri{"ldap://127.0.0.1:389"};
  std::string host{"127.0.0.1"};
  port_t port{kLdapDefaultPort};
  bool ldaps{false};
  bool starttls{false};
  std::string ca_file;
  std::string bind_dn;
  std::string password;
  std::string base_dn;
  std::string filter{"(objectClass=*)"};
  std::vector<std::string> attributes;
  SearchScope scope{SearchScope::Subtree};
  std::string ldif_file;
  std::vector<std::string> positionals;
  bool simple_auth{true};
  std::string sasl_mechanism;
  std::string sasl_authcid;
  std::string keytab;
  std::string new_password;
  std::string old_password;
  bool prompt_password{false};
  bool prompt_new_password{false};
  bool add_mode{false};
  bool help{false};
  bool version{false};
  bool parse_error{false};
};

void printClientUsage(const std::string &tool, const std::string &summary);
void printPasswdUsage();
void printVersion(const std::string &tool);
ClientOptions parseClientArgs(int argc, char *argv[], bool passwd = false);
int notImplementedExit(const std::string &tool);

}  // namespace cli
}  // namespace simple_ldapd
