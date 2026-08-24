/**
 * @file ldapwhoami.cpp
 * @brief RFC 4532 Who Am I
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/cli/common.hpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  using simple_ldapd::ResultCode;
  using simple_ldapd::cli::LdapClient;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::toString;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldapwhoami", "Print the bound authorization identity");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapwhoami", "Print the bound authorization identity");
    return 0;
  }
  if (options.version) {
    printVersion("ldapwhoami");
    return 0;
  }

  std::string error;
  auto client = LdapClient::openBound(options, error);
  if (!client) {
    std::cerr << "ldapwhoami: " << error << std::endl;
    return 1;
  }
  std::string authzid;
  const ResultCode result = client->whoAmI(authzid);
  client->unbind();
  if (result != ResultCode::Success) {
    std::cerr << "ldapwhoami: " << toString(result) << std::endl;
    return static_cast<int>(result);
  }
  if (authzid.empty()) {
    std::cout << "anonymous" << std::endl;
  } else {
    std::cout << authzid << std::endl;
  }
  return 0;
}
