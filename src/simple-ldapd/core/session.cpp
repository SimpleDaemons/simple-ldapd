/**
 * @file session.cpp
 * @brief LDAP bind/search session
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/core/session.hpp"

#include "simple-ldapd/auth/bind.hpp"
#include "simple-ldapd/utils/dn.hpp"

namespace simple_ldapd {

namespace {

bool isUserPassword(const std::string &name) { return iequals(name, "userPassword"); }

bool wantsAllAttributes(const std::vector<std::string> &names) {
  if (names.empty()) {
    return true;
  }
  for (const auto &name : names) {
    if (name == "*") {
      return true;
    }
  }
  return false;
}

bool wantsNoAttributes(const std::vector<std::string> &names) {
  return names.size() == 1 && names.front() == "1.1";
}

bool attributeRequested(const std::vector<std::string> &names,
                        const std::string &attribute) {
  for (const auto &name : names) {
    if (iequals(name, attribute)) {
      return true;
    }
  }
  return false;
}

}  // namespace

Session::Session(TcpConnection connection, Backend &backend, const LdapConfig &config,
                 std::atomic<bool> &running)
    : connection_(std::move(connection)), backend_(backend), config_(config),
      running_(running) {}

void Session::serve() {
  while (running_.load()) {
    if (!connection_.waitReadable(200)) {
      continue;
    }
    std::vector<uint8_t> pdu;
    if (!connection_.recvPdu(pdu)) {
      break;
    }
    const auto request = decodeLdapMessage(pdu);
    if (!request) {
      break;
    }
    if (request->op == ProtocolOp::UnbindRequest) {
      break;
    }
    if (request->op == ProtocolOp::SearchRequest) {
      if (!handleSearch(*request)) {
        break;
      }
      continue;
    }
    LdapMessage response;
    switch (request->op) {
    case ProtocolOp::BindRequest:
      response = handleBind(*request);
      break;
    case ProtocolOp::ModifyRequest:
      response = makeLdapResult(request->message_id, ProtocolOp::ModifyResponse,
                                ResultCode::UnwillingToPerform, "modify not implemented");
      break;
    case ProtocolOp::AddRequest:
      response = makeLdapResult(request->message_id, ProtocolOp::AddResponse,
                                ResultCode::UnwillingToPerform, "add not implemented");
      break;
    case ProtocolOp::DelRequest:
      response = makeLdapResult(request->message_id, ProtocolOp::DelResponse,
                                ResultCode::UnwillingToPerform, "delete not implemented");
      break;
    case ProtocolOp::ExtendedRequest:
      response =
          makeLdapResult(request->message_id, ProtocolOp::ExtendedResponse,
                         ResultCode::UnwillingToPerform, "extended operations not implemented");
      break;
    default:
      response = makeLdapResult(request->message_id, ProtocolOp::SearchResultDone,
                                ResultCode::ProtocolError, "unsupported operation");
      break;
    }
    if (!send(response)) {
      break;
    }
  }
}

bool Session::send(const LdapMessage &message) {
  return connection_.sendAll(encodeLdapMessage(message));
}

LdapMessage Session::handleBind(const LdapMessage &request) {
  bound_ = false;
  bind_dn_.clear();
  if (request.bind.version != 3) {
    return makeBindResponse(request.message_id, ResultCode::ProtocolError,
                            "only LDAPv3 is supported");
  }
  if (!request.bind.simple) {
    return makeBindResponse(request.message_id, ResultCode::AuthMethodNotSupported,
                            "SASL bind is not implemented");
  }
  SimpleBindAuthenticator authenticator(backend_, config_);
  const ResultCode result = authenticator.bind(request.bind.dn, request.bind.password);
  if (result == ResultCode::Success) {
    bound_ = true;
    bind_dn_ = request.bind.dn;
    return makeBindResponse(request.message_id, ResultCode::Success);
  }
  return makeBindResponse(request.message_id, result, "invalid credentials");
}

bool Session::handleSearch(const LdapMessage &request) {
  if (!request.search.filter.valid()) {
    return send(makeSearchDone(request.message_id, ResultCode::ProtocolError,
                               "invalid search filter"));
  }
  const std::string &base = request.search.base_dn;
  if (!base.empty() && !backend_.lookup(base)) {
    return send(makeSearchDone(request.message_id, ResultCode::NoSuchObject,
                               "no such object"));
  }
  auto matches = backend_.search(base, request.search.scope, request.search.filter);
  int sent = 0;
  for (const auto &entry : matches) {
    if (request.search.size_limit > 0 && sent >= request.search.size_limit) {
      return send(makeSearchDone(request.message_id, ResultCode::SizeLimitExceeded));
    }
    if (!send(makeSearchEntry(request.message_id, toSearchEntry(entry, request.search)))) {
      return false;
    }
    ++sent;
  }
  return send(makeSearchDone(request.message_id, ResultCode::Success));
}

SearchEntryData Session::toSearchEntry(const DirectoryEntry &entry,
                                       const SearchRequestData &request) const {
  SearchEntryData data;
  data.dn = entry.dn;
  if (wantsNoAttributes(request.attributes)) {
    return data;
  }
  const bool all = wantsAllAttributes(request.attributes);
  const bool reveal_password =
      bound_ && !bind_dn_.empty() && dnEquals(bind_dn_, config_.root_dn);
  for (const auto &pair : entry.attributes) {
    if (!all && !attributeRequested(request.attributes, pair.first)) {
      continue;
    }
    if (isUserPassword(pair.first) && !reveal_password) {
      continue;
    }
    PartialAttribute attribute;
    attribute.type = pair.first;
    if (!request.types_only) {
      attribute.values = pair.second;
    }
    data.attributes.push_back(std::move(attribute));
  }
  return data;
}

}  // namespace simple_ldapd
