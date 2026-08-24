/**
 * @file config.hpp
 * @brief Daemon configuration
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/utils/platform.hpp"
#include <string>
#include <vector>

namespace simple_ldapd {

class LdapConfig {
public:
  LdapConfig();

  bool loadFromFile(const std::string &path);
  bool validate() const;
  bool validateDetailed(std::vector<std::string> &errors) const;

  std::string listen_address{"0.0.0.0"};
  port_t ldap_port{kLdapDefaultPort};
  port_t ldaps_port{kLdapsDefaultPort};
  bool enable_ldaps{false};
  bool enable_starttls{false};
  std::string tls_cert_file;
  std::string tls_key_file;
  std::string tls_ca_file;
  std::string backend{"memory"};
  std::string ldif_file;
  std::string schema_dir{"schemas"};
  std::string base_dn{"dc=example,dc=com"};
  std::string root_dn{"cn=admin,dc=example,dc=com"};
  std::string log_file;
  std::string log_level{"info"};
  bool foreground{true};
};

}  // namespace simple_ldapd
