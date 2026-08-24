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
