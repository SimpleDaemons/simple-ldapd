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
      response = handleModify(*request);
      break;
    case ProtocolOp::AddRequest:
      response = handleAdd(*request);
      break;
    case ProtocolOp::DelRequest:
      response = handleDelete(*request);
      break;
    case ProtocolOp::ModifyDNRequest:
      response = handleModifyDn(*request);
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

bool Session::mayWrite() const {
  return bound_ && !bind_dn_.empty() && dnEquals(bind_dn_, config_.root_dn);
}

LdapMessage Session::handleAdd(const LdapMessage &request) {
  if (!mayWrite()) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::InsufficientAccessRights, "root bind required");
  }
  if (request.add.dn.empty()) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::InvalidDnSyntax, "empty DN");
  }
  if (backend_.lookup(request.add.dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::EntryAlreadyExists, "entry already exists");
  }
  const std::string parent = dnParent(request.add.dn);
  if (!parent.empty() && !backend_.lookup(parent)) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::NamingViolation, "parent does not exist");
  }
  if (!backend_.add(toDirectoryEntry(request.add))) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::OperationsError, "add failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::AddResponse, ResultCode::Success);
}

LdapMessage Session::handleModify(const LdapMessage &request) {
  if (!mayWrite()) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse,
                          ResultCode::InsufficientAccessRights, "root bind required");
  }
  auto entry = backend_.lookup(request.modify.dn);
  if (!entry) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  const ResultCode applied = applyModifications(*entry, request.modify.changes);
  if (applied != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse, applied,
                          toString(applied));
  }
  if (!backend_.modify(*entry)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse,
                          ResultCode::OperationsError, "modify failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse, ResultCode::Success);
}

LdapMessage Session::handleDelete(const LdapMessage &request) {
  if (!mayWrite()) {
    return makeLdapResult(request.message_id, ProtocolOp::DelResponse,
                          ResultCode::InsufficientAccessRights, "root bind required");
  }
  if (!backend_.lookup(request.delete_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::DelResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  if (backend_.hasChildren(request.delete_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::DelResponse,
                          ResultCode::NotAllowedOnNonLeaf, "entry has children");
  }
  if (!backend_.remove(request.delete_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::DelResponse,
                          ResultCode::OperationsError, "delete failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::DelResponse, ResultCode::Success);
}

LdapMessage Session::handleModifyDn(const LdapMessage &request) {
  if (!mayWrite()) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::InsufficientAccessRights, "root bind required");
  }
  auto entry = backend_.lookup(request.modify_dn.dn);
  if (!entry) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  if (request.modify_dn.new_rdn.empty()) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::InvalidDnSyntax, "empty new RDN");
  }
  const std::string parent = request.modify_dn.new_superior.empty()
                                 ? dnParent(request.modify_dn.dn)
                                 : request.modify_dn.new_superior;
  if (!parent.empty() && !backend_.lookup(parent)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::NamingViolation, "new parent does not exist");
  }
  const std::string new_dn = composeDn(request.modify_dn.new_rdn, parent);
  if (!dnEquals(new_dn, request.modify_dn.dn) && backend_.lookup(new_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::EntryAlreadyExists, "target DN exists");
  }
  std::string old_type;
  std::string old_value;
  if (request.modify_dn.delete_old_rdn &&
      parseRdn(dnRdn(request.modify_dn.dn), old_type, old_value)) {
    const ResultCode removed =
        applyModifications(*entry, {{ModifyOp::Delete, old_type, {old_value}}});
    if (removed != ResultCode::Success && removed != ResultCode::NoSuchAttribute) {
      return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse, removed,
                            toString(removed));
    }
  }
  std::string new_type;
  std::string new_value;
  if (parseRdn(request.modify_dn.new_rdn, new_type, new_value)) {
    const ResultCode added =
        applyModifications(*entry, {{ModifyOp::Add, new_type, {new_value}}});
    if (added != ResultCode::Success && added != ResultCode::AttributeOrValueExists) {
      return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse, added,
                            toString(added));
    }
  }
  if (!backend_.modify(*entry)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::OperationsError, "modify DN attributes failed");
  }
  if (!backend_.rename(request.modify_dn.dn, new_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::OperationsError, "rename failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse, ResultCode::Success);
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
