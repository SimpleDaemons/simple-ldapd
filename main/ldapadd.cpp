/**
 * @file ldapadd.cpp
 * @brief Add entries from LDIF
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/cli/common.hpp"
#include "simple-ldapd/protocol/ldif.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace {

std::string readInput(const std::string &path) {
  if (path.empty() || path == "-") {
    std::ostringstream out;
    out << std::cin.rdbuf();
    return out.str();
  }
  std::ifstream in(path);
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

}  // namespace

int main(int argc, char *argv[]) {
  using simple_ldapd::LdifChangeType;
  using simple_ldapd::ResultCode;
  using simple_ldapd::cli::LdapClient;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::parseLdifText;
  using simple_ldapd::toString;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldapadd", "Add entries from LDIF");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapadd", "Add entries from LDIF");
    return 0;
  }
  if (options.version) {
    printVersion("ldapadd");
    return 0;
  }

  const auto records = parseLdifText(readInput(options.ldif_file));
  if (records.empty()) {
    std::cerr << "ldapadd: no LDIF records" << std::endl;
    return 1;
  }
  std::string error;
  auto client = LdapClient::openBound(options, error);
  if (!client) {
    std::cerr << "ldapadd: " << error << std::endl;
    return 1;
  }
  ResultCode last = ResultCode::Success;
  for (const auto &record : records) {
    if (record.changetype != LdifChangeType::Content &&
        record.changetype != LdifChangeType::Add) {
      continue;
    }
    last = client->add(record.entry);
    if (last != ResultCode::Success) {
      std::cerr << "ldapadd: " << record.dn << ": " << toString(last) << std::endl;
      client->unbind();
      return static_cast<int>(last);
    }
  }
  client->unbind();
  return 0;
}
