/**
 * @file ldapcompare.cpp
 * @brief Compare an attribute value (OpenLDAP-compatible flags)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/cli/common.hpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  using simple_ldapd::CompareRequestData;
  using simple_ldapd::ResultCode;
  using simple_ldapd::cli::LdapClient;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::toString;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldapcompare", "Compare an attribute value against an entry");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapcompare", "Compare an attribute value against an entry");
    return 0;
  }
  if (options.version) {
    printVersion("ldapcompare");
    return 0;
  }
  if (options.positionals.size() < 2) {
    std::cerr << "ldapcompare: usage: ldapcompare [options] DN attr:value" << std::endl;
    return 1;
  }

  const std::string &dn = options.positionals[0];
  const std::string &assertion = options.positionals[1];
  const auto colon = assertion.find(':');
  if (colon == std::string::npos || colon == 0 || colon + 1 >= assertion.size()) {
    std::cerr << "ldapcompare: assertion must be attr:value" << std::endl;
    return 1;
  }

  std::string error;
  auto client = LdapClient::openBound(options, error);
  if (!client) {
    std::cerr << "ldapcompare: " << error << std::endl;
    return 1;
  }
  CompareRequestData request;
  request.dn = dn;
  request.attribute = assertion.substr(0, colon);
  request.value = assertion.substr(colon + 1);
  const ResultCode result = client->compare(request);
  client->unbind();
  if (result != ResultCode::CompareTrue && result != ResultCode::CompareFalse) {
    std::cerr << "ldapcompare: " << toString(result) << std::endl;
  } else {
    std::cout << (result == ResultCode::CompareTrue ? "TRUE" : "FALSE") << std::endl;
  }
  return static_cast<int>(result);
}
