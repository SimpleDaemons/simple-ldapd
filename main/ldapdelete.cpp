/**
 * @file ldapdelete.cpp
 * @brief Delete directory entries
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/cli/common.hpp"
#include "simple-ldapd/protocol/ldif.hpp"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  using simple_ldapd::LdifChangeType;
  using simple_ldapd::ResultCode;
  using simple_ldapd::cli::LdapClient;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::parseLdifFile;
  using simple_ldapd::toString;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldapdelete", "Delete directory entries");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapdelete", "Delete directory entries");
    return 0;
  }
  if (options.version) {
    printVersion("ldapdelete");
    return 0;
  }

  std::vector<std::string> dns = options.positionals;
  if (!options.ldif_file.empty()) {
    for (const auto &record : parseLdifFile(options.ldif_file)) {
      if (record.changetype == LdifChangeType::Delete ||
          record.changetype == LdifChangeType::Content) {
        dns.push_back(record.dn);
      }
    }
  }
  if (dns.empty()) {
    std::cerr << "ldapdelete: no DNs specified" << std::endl;
    return 1;
  }

  std::string error;
  auto client = LdapClient::openBound(options, error);
  if (!client) {
    std::cerr << "ldapdelete: " << error << std::endl;
    return 1;
  }
  for (const auto &dn : dns) {
    const ResultCode result = client->del(dn);
    if (result != ResultCode::Success) {
      std::cerr << "ldapdelete: " << dn << ": " << toString(result) << std::endl;
      client->unbind();
      return static_cast<int>(result);
    }
  }
  client->unbind();
  return 0;
}
