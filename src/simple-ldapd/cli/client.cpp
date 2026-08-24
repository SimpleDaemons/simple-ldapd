/**
 * @file client.cpp
 * @brief Minimal LDAPv3 client helper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"

#include "simple-ldapd/security/tls.hpp"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace simple_ldapd {
namespace cli {

std::optional<TcpConnection> connectLdap(const ClientOptions &options, std::string &error) {
  auto connection = TcpConnection::connectTo(options.host, options.port);
  if (!connection) {
    error = "cannot connect to " + options.host + ":" + std::to_string(options.port);
    return std::nullopt;
  }
  const bool want_tls = options.ldaps || options.starttls;
  if (!want_tls) {
    return connection;
  }
  TlsContext tls;
  if (!tls.initClient(options.ca_file)) {
    error = "TLS client initialization failed";
    return std::nullopt;
  }
  if (options.ldaps) {
    if (!connection->handshakeTls(tls, false, options.host)) {
      error = "LDAPS handshake failed";
      return std::nullopt;
    }
    return connection;
  }
  if (!connection->sendAll(encodeLdapMessage(makeExtendedRequest(1, kStartTlsOid)))) {
    error = "StartTLS request failed";
    return std::nullopt;
  }
  std::vector<uint8_t> pdu;
  if (!connection->recvPdu(pdu)) {
    error = "StartTLS response truncated";
    return std::nullopt;
  }
  const auto response = decodeLdapMessage(pdu);
  if (!response || response->op != ProtocolOp::ExtendedResponse ||
      response->result != ResultCode::Success) {
    error = response ? toString(response->result) : "StartTLS failed";
    return std::nullopt;
  }
  if (!connection->handshakeTls(tls, false, options.host)) {
    error = "StartTLS handshake failed";
    return std::nullopt;
  }
  return connection;
}

LdapClient::LdapClient(TcpConnection connection) : connection_(std::move(connection)) {}

std::optional<LdapClient> LdapClient::connect(const std::string &host, port_t port) {
  auto connection = TcpConnection::connectTo(host, port);
  if (!connection) {
    return std::nullopt;
  }
  return LdapClient(std::move(*connection));
}

std::optional<LdapClient> LdapClient::openBound(const ClientOptions &options, std::string &error) {
  auto connection = connectLdap(options, error);
  if (!connection) {
    return std::nullopt;
  }
  LdapClient client(std::move(*connection));
  std::string password = options.password;
  if (options.prompt_password) {
    std::cerr << "Enter LDAP Password: ";
    std::getline(std::cin, password);
  }
  const ResultCode bound = client.simpleBind(options.bind_dn, password);
  if (bound != ResultCode::Success) {
    error = toString(bound);
    return std::nullopt;
  }
  return client;
}

std::optional<LdapMessage> LdapClient::exchange(const LdapMessage &request) {
  if (!connection_.sendAll(encodeLdapMessage(request))) {
    return std::nullopt;
  }
  std::vector<uint8_t> pdu;
  if (!connection_.recvPdu(pdu)) {
    return std::nullopt;
  }
  return decodeLdapMessage(pdu);
}

ResultCode LdapClient::simpleBind(const std::string &dn, const std::string &password) {
  auto response = exchange(makeBindRequest(next_id_++, dn, password));
  if (!response || response->op != ProtocolOp::BindResponse) {
    return ResultCode::Unavailable;
  }
  return response->result;
}

ResultCode LdapClient::add(const DirectoryEntry &entry) {
  auto response = exchange(makeAddRequest(next_id_++, toAddRequest(entry)));
  if (!response || response->op != ProtocolOp::AddResponse) {
    return ResultCode::Unavailable;
  }
  return response->result;
}

ResultCode LdapClient::modify(const std::string &dn,
                              const std::vector<AttributeModification> &changes) {
  ModifyRequestData request;
  request.dn = dn;
  request.changes = changes;
  auto response = exchange(makeModifyRequest(next_id_++, request));
  if (!response || response->op != ProtocolOp::ModifyResponse) {
    return ResultCode::Unavailable;
  }
  return response->result;
}

ResultCode LdapClient::del(const std::string &dn) {
  auto response = exchange(makeDelRequest(next_id_++, dn));
  if (!response || response->op != ProtocolOp::DelResponse) {
    return ResultCode::Unavailable;
  }
  return response->result;
}

ResultCode LdapClient::modifyDn(const ModifyDnRequestData &request) {
  auto response = exchange(makeModifyDnRequest(next_id_++, request));
  if (!response || response->op != ProtocolOp::ModifyDNResponse) {
    return ResultCode::Unavailable;
  }
  return response->result;
}

void LdapClient::unbind() {
  connection_.sendAll(encodeLdapMessage(makeUnbindRequest(next_id_++)));
}

}  // namespace cli
}  // namespace simple_ldapd
