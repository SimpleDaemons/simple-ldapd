/**
 * @file ldapsearch.cpp
 * @brief Search the directory (OpenLDAP-compatible flags)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/cli/common.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/utils/net.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

bool recvMessage(simple_ldapd::TcpConnection &connection,
                 simple_ldapd::LdapMessage &message) {
  std::vector<uint8_t> pdu;
  if (!connection.recvPdu(pdu)) {
    return false;
  }
  auto decoded = simple_ldapd::decodeLdapMessage(pdu);
  if (!decoded) {
    return false;
  }
  message = std::move(*decoded);
  return true;
}

void printEntry(const simple_ldapd::SearchEntryData &entry) {
  std::cout << "dn: " << entry.dn << "\n";
  for (const auto &attribute : entry.attributes) {
    if (attribute.values.empty()) {
      std::cout << attribute.type << ":\n";
      continue;
    }
    for (const auto &value : attribute.values) {
      std::cout << attribute.type << ": " << value << "\n";
    }
  }
  std::cout << "\n";
}

}  // namespace

int main(int argc, char *argv[]) {
  using simple_ldapd::ResultCode;
  using simple_ldapd::SearchFilter;
  using simple_ldapd::SearchRequestData;
  using simple_ldapd::TcpConnection;
  using simple_ldapd::cli::bindLdap;
  using simple_ldapd::cli::connectLdap;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printClientUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::encodeLdapMessage;
  using simple_ldapd::makeSearchRequest;
  using simple_ldapd::makeUnbindRequest;

  const auto options = parseClientArgs(argc, argv);
  if (options.parse_error) {
    printClientUsage("ldapsearch", "Search the directory (OpenLDAP-compatible flags)");
    return 1;
  }
  if (options.help) {
    printClientUsage("ldapsearch", "Search the directory (OpenLDAP-compatible flags)");
    return 0;
  }
  if (options.version) {
    printVersion("ldapsearch");
    return 0;
  }

  std::string password = options.password;
  if (options.prompt_password) {
    std::cerr << "Enter LDAP Password: ";
    std::getline(std::cin, password);
  }

  auto filter = SearchFilter::parse(options.filter);
  if (!filter.valid()) {
    std::cerr << "ldapsearch: invalid filter: " << options.filter << std::endl;
    return 1;
  }

  std::string error;
  auto connection = connectLdap(options, error);
  if (!connection) {
    std::cerr << "ldapsearch: " << error << std::endl;
    return 1;
  }

  int message_id = 1;
  const ResultCode bound = bindLdap(*connection, message_id, options, password, error);
  if (bound != ResultCode::Success) {
    std::cerr << "ldapsearch: " << (error.empty() ? toString(bound) : error) << std::endl;
    return static_cast<int>(bound);
  }

  SearchRequestData search;
  search.base_dn = options.base_dn;
  search.scope = options.scope;
  search.filter = std::move(filter);
  search.attributes = options.attributes;
  if (!connection->sendAll(encodeLdapMessage(makeSearchRequest(message_id++, search)))) {
    std::cerr << "ldapsearch: search send failed" << std::endl;
    return 1;
  }

  ResultCode search_result = ResultCode::OperationsError;
  while (true) {
    simple_ldapd::LdapMessage message;
    if (!recvMessage(*connection, message)) {
      std::cerr << "ldapsearch: search response truncated" << std::endl;
      return 1;
    }
    if (message.op == simple_ldapd::ProtocolOp::SearchResultEntry) {
      printEntry(message.entry);
      continue;
    }
    if (message.op == simple_ldapd::ProtocolOp::SearchResultDone) {
      search_result = message.result;
      if (search_result != ResultCode::Success) {
        std::cerr << "ldapsearch: " << toString(search_result);
        if (!message.diagnostic.empty()) {
          std::cerr << " (" << message.diagnostic << ")";
        }
        std::cerr << std::endl;
      }
      break;
    }
    std::cerr << "ldapsearch: unexpected search response" << std::endl;
    return 1;
  }

  connection->sendAll(encodeLdapMessage(makeUnbindRequest(message_id++)));
  return static_cast<int>(search_result);
}
