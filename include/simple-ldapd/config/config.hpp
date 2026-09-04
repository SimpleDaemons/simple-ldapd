/**
 * @file config.hpp
 * @brief Daemon configuration
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/utils/platform.hpp"
#include "simple-ldapd/security/acl.hpp"
#include <cstdint>
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
  bool tls_verify_client{false};
  std::string backend{"memory"};
  std::string ldif_file;
  std::string sqlite_file;
  std::string schema_dir{"schemas"};
  std::string base_dn{"dc=example,dc=com"};
  std::string root_dn{"cn=admin,dc=example,dc=com"};
  std::string root_password;
  std::string log_file;
  std::string log_level{"info"};
  std::uint32_t bind_rate_limit{0};
  std::uint32_t max_pdu_size{1024 * 1024};
  std::uint32_t max_sessions{0};
  std::uint32_t idle_timeout{0};
  bool foreground{true};
  bool require_confidentiality{false};
  std::string krb_realm;
  std::string gssapi_keytab;
  std::string gssapi_service;
  std::vector<AclRule> acls;
  std::vector<std::string> acl_errors;
};

}  // namespace simple_ldapd
