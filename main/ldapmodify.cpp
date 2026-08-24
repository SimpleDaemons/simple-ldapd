/**
 * @file ldapmodify.cpp
 * @brief Modify entries from LDIF
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
  using simple_ldapd::AttributeModification;
  using simple_ldapd::LdifChangeType;
  using simple_ldapd::ModifyDnRequestData;
  using simple_ldapd::ModifyOp;
  using simple_ldapd::ResultCode;
  using simple_ldapd::cli::LdapClient;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::parseLdifText;
  using simple_ldapd::toString;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldapmodify", "Modify entries from LDIF");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapmodify", "Modify entries from LDIF");
    return 0;
  }
  if (options.version) {
    printVersion("ldapmodify");
    return 0;
  }

  const auto records = parseLdifText(readInput(options.ldif_file));
  if (records.empty()) {
    std::cerr << "ldapmodify: no LDIF records" << std::endl;
    return 1;
  }
  std::string error;
  auto client = LdapClient::openBound(options, error);
  if (!client) {
    std::cerr << "ldapmodify: " << error << std::endl;
    return 1;
  }

  for (const auto &record : records) {
    LdifChangeType type = record.changetype;
    if (type == LdifChangeType::Content) {
      type = options.add_mode ? LdifChangeType::Add : LdifChangeType::Modify;
    }
    ResultCode result = ResultCode::UnwillingToPerform;
    if (type == LdifChangeType::Add) {
      result = client->add(record.entry);
    } else if (type == LdifChangeType::Delete) {
      result = client->del(record.dn);
    } else if (type == LdifChangeType::Modrdn) {
      ModifyDnRequestData request;
      request.dn = record.dn;
      request.new_rdn = record.new_rdn;
      request.delete_old_rdn = record.delete_old_rdn;
      request.new_superior = record.new_superior;
      result = client->modifyDn(request);
    } else {
      auto changes = record.changes;
      if (changes.empty()) {
        for (const auto &pair : record.entry.attributes) {
          changes.push_back(AttributeModification{ModifyOp::Replace, pair.first, pair.second});
        }
      }
      result = client->modify(record.dn, changes);
    }
    if (result != ResultCode::Success) {
      std::cerr << "ldapmodify: " << record.dn << ": " << toString(result) << std::endl;
      client->unbind();
      return static_cast<int>(result);
    }
  }
  client->unbind();
  return 0;
}
