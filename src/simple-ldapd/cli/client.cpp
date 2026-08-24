/**
 * @file client.cpp
 * @brief Minimal LDAPv3 client helper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"

#include "simple-ldapd/auth/sasl.hpp"
#include "simple-ldapd/auth/gssapi.hpp"
#include "simple-ldapd/security/tls.hpp"
#include <cstdlib>
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

ResultCode bindLdap(TcpConnection &connection, int &message_id, const ClientOptions &options,
                    const std::string &password, std::string &error) {
  auto sendBind = [&](const LdapMessage &request) -> std::optional<LdapMessage> {
    if (!connection.sendAll(encodeLdapMessage(request))) {
      error = "bind send failed";
      return std::nullopt;
    }
    std::vector<uint8_t> pdu;
    if (!connection.recvPdu(pdu)) {
      error = "bind response truncated";
      return std::nullopt;
    }
    auto decoded = decodeLdapMessage(pdu);
    if (!decoded || decoded->op != ProtocolOp::BindResponse) {
      error = "bind failed";
      return std::nullopt;
    }
    return decoded;
  };

  if (options.sasl_mechanism.empty()) {
    auto response = sendBind(makeBindRequest(message_id++, options.bind_dn, password));
    if (!response) {
      return ResultCode::Unavailable;
    }
    if (response->result != ResultCode::Success) {
      error = toString(response->result);
      return response->result;
    }
    return ResultCode::Success;
  }

  SaslMechanism mechanism = SaslMechanism::Anonymous;
  if (!parseSaslMechanism(options.sasl_mechanism, mechanism)) {
    error = "unsupported SASL mechanism";
    return ResultCode::AuthMethodNotSupported;
  }
  const std::string authcid =
      !options.sasl_authcid.empty() ? options.sasl_authcid : options.bind_dn;
  std::string credentials;
  if (mechanism == SaslMechanism::Plain) {
    credentials = saslPlainCredentials({}, authcid, password);
  } else if (mechanism == SaslMechanism::External) {
    credentials = authcid;
  } else if (mechanism == SaslMechanism::DigestMd5) {
    auto challenge = sendBind(makeSaslBindRequest(message_id++, options.bind_dn, "DIGEST-MD5"));
    if (!challenge) {
      return ResultCode::Unavailable;
    }
    if (challenge->result != ResultCode::SaslBindInProgress) {
      error = toString(challenge->result);
      return challenge->result;
    }
    if (!saslDigestClientResponse(challenge->server_sasl_creds, authcid, password,
                                  "ldap/" + options.host, credentials, error)) {
      return ResultCode::AuthMethodNotSupported;
    }
  } else if (mechanism == SaslMechanism::Gssapi) {
    std::string keytab_path = options.keytab;
    if (keytab_path.empty()) {
      if (const char *env = std::getenv("SIMPLE_LDAPD_KTNAME")) {
        keytab_path = env;
      }
    }
    if (keytab_path.empty()) {
      error = "GSSAPI requires --keytab";
      return ResultCode::AuthMethodNotSupported;
    }
    GssapiKeytab keytab;
    if (!loadGssapiKeytab(keytab_path, keytab, error)) {
      return ResultCode::OperationsError;
    }
    if (keytab.realm.empty()) {
      error = "GSSAPI keytab is missing realm";
      return ResultCode::OperationsError;
    }
    auto challenge = sendBind(makeSaslBindRequest(message_id++, options.bind_dn, "GSSAPI"));
    if (!challenge) {
      return ResultCode::Unavailable;
    }
    if (challenge->result != ResultCode::SaslBindInProgress) {
      error = toString(challenge->result);
      return challenge->result;
    }
    if (authcid.empty()) {
      error = "GSSAPI requires -U";
      return ResultCode::InvalidCredentials;
    }
    credentials = mintGssapiTicket(keytab, authcid);
    if (credentials.empty()) {
      error = "GSSAPI ticket mint failed";
      return ResultCode::OperationsError;
    }
  } else {
    error = "SASL mechanism is not available in the client";
    return ResultCode::AuthMethodNotSupported;
  }
  auto response =
      sendBind(makeSaslBindRequest(message_id++, options.bind_dn, options.sasl_mechanism,
                                   credentials));
  if (!response) {
    return ResultCode::Unavailable;
  }
  if (response->result != ResultCode::Success) {
    error = toString(response->result);
    return response->result;
  }
  return ResultCode::Success;
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
  std::string password = options.password;
  if (options.prompt_password) {
    std::cerr << "Enter LDAP Password: ";
    std::getline(std::cin, password);
  }
  int message_id = 1;
  const ResultCode bound = bindLdap(*connection, message_id, options, password, error);
  if (bound != ResultCode::Success) {
    if (error.empty()) {
      error = toString(bound);
    }
    return std::nullopt;
  }
  return LdapClient(std::move(*connection));
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

ResultCode LdapClient::saslBind(const std::string &mechanism, const std::string &dn,
                                const std::string &credentials) {
  auto response = exchange(makeSaslBindRequest(next_id_++, dn, mechanism, credentials));
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

ResultCode LdapClient::passwordModify(const PasswordModifyRequest &request) {
  auto response = exchange(makePasswordModifyRequest(next_id_++, request));
  if (!response || response->op != ProtocolOp::ExtendedResponse) {
    return ResultCode::Unavailable;
  }
  return response->result;
}

void LdapClient::unbind() {
  connection_.sendAll(encodeLdapMessage(makeUnbindRequest(next_id_++)));
}

}  // namespace cli
}  // namespace simple_ldapd
