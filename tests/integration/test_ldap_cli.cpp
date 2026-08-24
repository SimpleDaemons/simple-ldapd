/**
 * @file test_ldap_cli.cpp
 * @brief CLI option parsing tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/common.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"

#include <iostream>
#include <vector>

using namespace simple_ldapd;

int main() {
  std::vector<char *> args{
      const_cast<char *>("ldapsearch"),
      const_cast<char *>("-H"),
      const_cast<char *>("ldap://127.0.0.1:3389"),
      const_cast<char *>("-x"),
      const_cast<char *>("-b"),
      const_cast<char *>("dc=example,dc=com"),
      const_cast<char *>("(uid=alice)"),
  };
  auto options = cli::parseClientArgs(static_cast<int>(args.size()), args.data());
  if (options.uri != "ldap://127.0.0.1:3389" || options.base_dn != "dc=example,dc=com" ||
      options.filter != "(uid=alice)" || options.host != "127.0.0.1" ||
      options.port != 3389) {
    std::cout << "FAIL parseClientArgs" << std::endl;
    return 1;
  }

  std::vector<char *> tls_args{
      const_cast<char *>("ldapsearch"),
      const_cast<char *>("-H"),
      const_cast<char *>("ldaps://127.0.0.1:6636"),
      const_cast<char *>("-Z"),
      const_cast<char *>("--ca-file"),
      const_cast<char *>("ca.crt"),
  };
  auto tls_options =
      cli::parseClientArgs(static_cast<int>(tls_args.size()), tls_args.data());
  if (!tls_options.ldaps || tls_options.port != 6636 || !tls_options.starttls ||
      tls_options.ca_file != "ca.crt") {
    std::cout << "FAIL parseTlsArgs" << std::endl;
    return 1;
  }

  std::vector<char *> sasl_args{
      const_cast<char *>("ldapsearch"),
      const_cast<char *>("-H"),
      const_cast<char *>("ldap://127.0.0.1:3389"),
      const_cast<char *>("-Y"),
      const_cast<char *>("PLAIN"),
      const_cast<char *>("-U"),
      const_cast<char *>("alice"),
      const_cast<char *>("-w"),
      const_cast<char *>("secret"),
  };
  auto sasl_options =
      cli::parseClientArgs(static_cast<int>(sasl_args.size()), sasl_args.data());
  if (sasl_options.simple_auth || sasl_options.sasl_mechanism != "PLAIN" ||
      sasl_options.sasl_authcid != "alice") {
    std::cout << "FAIL parseSaslArgs" << std::endl;
    return 1;
  }

  std::vector<char *> gssapi_args{
      const_cast<char *>("ldapsearch"),
      const_cast<char *>("-H"),
      const_cast<char *>("ldap://127.0.0.1:3389"),
      const_cast<char *>("-Y"),
      const_cast<char *>("GSSAPI"),
      const_cast<char *>("-U"),
      const_cast<char *>("alice"),
      const_cast<char *>("--keytab"),
      const_cast<char *>("lab.keytab"),
  };
  auto gssapi_options =
      cli::parseClientArgs(static_cast<int>(gssapi_args.size()), gssapi_args.data());
  if (gssapi_options.simple_auth || gssapi_options.sasl_mechanism != "GSSAPI" ||
      gssapi_options.sasl_authcid != "alice" || gssapi_options.keytab != "lab.keytab") {
    std::cout << "FAIL parseGssapiArgs" << std::endl;
    return 1;
  }

  std::vector<char *> passwd_args{
      const_cast<char *>("ldappasswd"),
      const_cast<char *>("-H"),
      const_cast<char *>("ldap://127.0.0.1:3389"),
      const_cast<char *>("-x"),
      const_cast<char *>("-D"),
      const_cast<char *>("cn=admin,dc=example,dc=com"),
      const_cast<char *>("-w"),
      const_cast<char *>("secret"),
      const_cast<char *>("-s"),
      const_cast<char *>("new-secret"),
      const_cast<char *>("uid=alice,ou=People,dc=example,dc=com"),
  };
  auto passwd_options =
      cli::parseClientArgs(static_cast<int>(passwd_args.size()), passwd_args.data(), true);
  if (passwd_options.new_password != "new-secret" ||
      passwd_options.positionals.size() != 1 ||
      passwd_options.positionals.front() != "uid=alice,ou=People,dc=example,dc=com") {
    std::cout << "FAIL parsePasswdArgs" << std::endl;
    return 1;
  }

  LdapConfig config;
  config.ldap_port = 3389;
  LdapDaemon daemon(config);
  if (!daemon.testConfig()) {
    std::cout << "FAIL testConfig" << std::endl;
    return 1;
  }
  std::cout << "PASS integration CLI/config" << std::endl;
  return 0;
}
