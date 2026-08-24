/**
 * @file session.cpp
 * @brief LDAP bind/search session
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/core/session.hpp"

#include "simple-ldapd/auth/bind.hpp"
#include "simple-ldapd/auth/password.hpp"
#include "simple-ldapd/auth/sasl.hpp"
#include "simple-ldapd/schema/registry.hpp"
#include "simple-ldapd/security/acl.hpp"
#include "simple-ldapd/security/tls.hpp"
#include "simple-ldapd/utils/dn.hpp"
#include "simple-ldapd/version.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <optional>

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
                 std::atomic<bool> &running, TlsContext *tls, SchemaRegistry *schema,
                 SaslAuthenticator *sasl)
    : connection_(std::move(connection)), backend_(backend), config_(config),
      running_(running), tls_(tls), schema_(schema), sasl_(sasl) {}

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
    case ProtocolOp::CompareRequest:
      response = handleCompare(*request);
      break;
    case ProtocolOp::ExtendedRequest:
      if (request->extended_oid == kStartTlsOid) {
        if (!config_.enable_starttls || tls_ == nullptr || !tls_->enabled() ||
            connection_.tls()) {
          response = makeLdapResult(request->message_id, ProtocolOp::ExtendedResponse,
                                    ResultCode::UnwillingToPerform,
                                    connection_.tls() ? "TLS already active"
                                                      : "StartTLS is not available");
          break;
        }
        if (!send(makeLdapResult(request->message_id, ProtocolOp::ExtendedResponse,
                                 ResultCode::Success))) {
          return;
        }
        if (!connection_.handshakeTls(*tls_, true)) {
          return;
        }
        continue;
      }
      if (request->extended_oid == kPasswordModifyOid) {
        response = handlePasswordModify(*request);
        break;
      }
      if (request->extended_oid == kWhoAmIOid) {
        response = handleWhoAmI(*request);
        break;
      }
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
    if (sasl_ == nullptr) {
      return makeBindResponse(request.message_id, ResultCode::AuthMethodNotSupported,
                              "SASL bind is not implemented");
    }
    const auto outcome =
        sasl_->bind(backend_, config_, request.bind, connection_.tls(), sasl_digest_nonce_);
    if (outcome.result == ResultCode::Success) {
      bound_ = true;
      bind_dn_ = outcome.bind_dn;
    }
    return makeBindResponse(request.message_id, outcome.result, outcome.diagnostic,
                            outcome.server_creds);
  }
  sasl_digest_nonce_.clear();
  if (config_.require_confidentiality && !connection_.tls() &&
      !request.bind.password.empty()) {
    return makeBindResponse(request.message_id, ResultCode::ConfidentialityRequired,
                            "confidentiality required");
  }
  SimpleBindAuthenticator authenticator(backend_, config_);
  const ResultCode result = authenticator.bind(request.bind.dn, request.bind.password);
  if (result == ResultCode::Success) {
    bound_ = true;
    if (request.bind.dn.empty()) {
      bind_dn_.clear();
    } else {
      bind_dn_ = authenticator.resolveName(request.bind.dn).value_or(request.bind.dn);
    }
    return makeBindResponse(request.message_id, ResultCode::Success);
  }
  return makeBindResponse(request.message_id, result, "invalid credentials");
}

bool Session::handleSearch(const LdapMessage &request) {
  if (!request.search.filter.valid()) {
    return send(makeSearchDone(request.message_id, ResultCode::ProtocolError,
                               "invalid search filter"));
  }
  int page_size = -1;
  std::string cookie;
  for (const auto &control : request.controls) {
    if (iequals(control.oid, kPagedResultsOid)) {
      if (!decodePagedResultsValue(control.value, page_size, cookie)) {
        return send(makeSearchDone(request.message_id,
                                   control.critical ? ResultCode::UnavailableCriticalExtension
                                                    : ResultCode::ProtocolError,
                                   "invalid paged results control"));
      }
      continue;
    }
    if (control.critical) {
      return send(makeSearchDone(request.message_id, ResultCode::UnavailableCriticalExtension,
                                 "unsupported critical control"));
    }
  }
  const std::string &base = request.search.base_dn;
  if (!base.empty() && !backend_.lookup(base)) {
    return send(makeSearchDone(request.message_id, ResultCode::NoSuchObject,
                               "no such object"));
  }
  if (!base.empty() && !maySearch(base)) {
    return send(makeSearchDone(request.message_id, ResultCode::InsufficientAccessRights,
                               "insufficient access"));
  }
  std::vector<DirectoryEntry> matches;
  if (base.empty() && request.search.scope != SearchScope::OneLevel) {
    DirectoryEntry dse = rootDse();
    if (request.search.filter.matches(dse)) {
      matches.push_back(std::move(dse));
    }
    if (request.search.scope == SearchScope::Base) {
      // Root DSE only.
    } else {
      auto rest = backend_.search(base, request.search.scope, request.search.filter);
      matches.insert(matches.end(), rest.begin(), rest.end());
    }
  } else {
    matches = backend_.search(base, request.search.scope, request.search.filter);
  }
  std::vector<DirectoryEntry> visible;
  for (auto &entry : matches) {
    if (maySearch(entry.dn)) {
      visible.push_back(std::move(entry));
    }
  }
  int offset = 0;
  if (!cookie.empty()) {
    char *end = nullptr;
    const long parsed = std::strtol(cookie.c_str(), &end, 10);
    if (end == cookie.c_str() || (end != nullptr && *end != '\0') || parsed < 0) {
      return send(makeSearchDone(request.message_id, ResultCode::ProtocolError,
                                 "invalid paged results cookie"));
    }
    offset = static_cast<int>(parsed);
  }
  const auto deadline =
      request.search.time_limit > 0
          ? std::optional<std::chrono::steady_clock::time_point>(
                std::chrono::steady_clock::now() +
                std::chrono::seconds(request.search.time_limit))
          : std::nullopt;
  int sent = 0;
  ResultCode done_result = ResultCode::Success;
  if (page_size == 0) {
    LdapMessage done = makeSearchDone(request.message_id, ResultCode::Success);
    done.controls.push_back(makePagedResultsControl(0, {}));
    return send(done);
  }
  for (int i = offset; i < static_cast<int>(visible.size()); ++i) {
    if (deadline && std::chrono::steady_clock::now() >= *deadline) {
      done_result = ResultCode::TimeLimitExceeded;
      break;
    }
    if (request.search.size_limit > 0 && sent >= request.search.size_limit) {
      done_result = ResultCode::SizeLimitExceeded;
      break;
    }
    if (page_size > 0 && sent >= page_size) {
      break;
    }
    if (!send(makeSearchEntry(request.message_id, toSearchEntry(visible[static_cast<size_t>(i)],
                                                               request.search)))) {
      return false;
    }
    ++sent;
  }
  LdapMessage done = makeSearchDone(request.message_id, done_result);
  if (page_size > 0) {
    const int next = offset + sent;
    const std::string next_cookie =
        next < static_cast<int>(visible.size()) && done_result != ResultCode::SizeLimitExceeded
            ? std::to_string(next)
            : std::string{};
    const int remaining = std::max(0, static_cast<int>(visible.size()) - next);
    done.controls.push_back(makePagedResultsControl(remaining, next_cookie));
  }
  return send(done);
}

DirectoryEntry Session::rootDse() const {
  DirectoryEntry entry;
  entry.attributes["objectClass"].push_back("top");
  entry.attributes["namingContexts"].push_back(config_.base_dn);
  entry.attributes["supportedLDAPVersion"].push_back("3");
  entry.attributes["vendorName"].push_back("SimpleDaemons");
  entry.attributes["vendorVersion"].push_back(kVersion);
  entry.attributes["supportedExtension"].push_back(kPasswordModifyOid);
  entry.attributes["supportedExtension"].push_back(kWhoAmIOid);
  entry.attributes["supportedControl"].push_back(kPagedResultsOid);
  if (config_.enable_starttls) {
    entry.attributes["supportedExtension"].push_back(kStartTlsOid);
  }
  if (sasl_ != nullptr) {
    entry.attributes["supportedSASLMechanisms"] = sasl_->advertised();
  }
  return entry;
}

bool Session::maySearch(const std::string &entry_dn) const {
  return AccessControl(config_).maySearch(bind_dn_, entry_dn, backend_);
}

bool Session::mayWrite(const std::string &entry_dn) const {
  return bound_ && AccessControl(config_).mayWrite(bind_dn_, entry_dn, backend_);
}

ResultCode Session::checkSchema(const DirectoryEntry &entry, std::string &diagnostic) const {
  if (schema_ == nullptr) {
    return ResultCode::Success;
  }
  return schema_->validateEntry(entry, diagnostic);
}

LdapMessage Session::handleAdd(const LdapMessage &request) {
  if (!mayWrite(request.add.dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::InsufficientAccessRights, "insufficient access");
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
  DirectoryEntry entry = toDirectoryEntry(request.add);
  encodeUserPasswords(entry);
  std::string diagnostic;
  const ResultCode schema = checkSchema(entry, diagnostic);
  if (schema != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse, schema, diagnostic);
  }
  if (!backend_.add(entry)) {
    return makeLdapResult(request.message_id, ProtocolOp::AddResponse,
                          ResultCode::OperationsError, "add failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::AddResponse, ResultCode::Success);
}

LdapMessage Session::handleModify(const LdapMessage &request) {
  if (!mayWrite(request.modify.dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse,
                          ResultCode::InsufficientAccessRights, "insufficient access");
  }
  auto entry = backend_.lookup(request.modify.dn);
  if (!entry) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  auto changes = request.modify.changes;
  encodeUserPasswordChanges(changes);
  const ResultCode applied = applyModifications(*entry, changes);
  if (applied != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse, applied,
                          toString(applied));
  }
  std::string diagnostic;
  const ResultCode schema = checkSchema(*entry, diagnostic);
  if (schema != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse, schema, diagnostic);
  }
  if (!backend_.modify(*entry)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse,
                          ResultCode::OperationsError, "modify failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::ModifyResponse, ResultCode::Success);
}

LdapMessage Session::handleDelete(const LdapMessage &request) {
  if (!mayWrite(request.delete_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::DelResponse,
                          ResultCode::InsufficientAccessRights, "insufficient access");
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
  if (request.modify_dn.new_rdn.empty()) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::InvalidDnSyntax, "empty new RDN");
  }
  const std::string parent = request.modify_dn.new_superior.empty()
                                 ? dnParent(request.modify_dn.dn)
                                 : request.modify_dn.new_superior;
  const std::string new_dn = composeDn(request.modify_dn.new_rdn, parent);
  if (!mayWrite(request.modify_dn.dn) || !mayWrite(new_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::InsufficientAccessRights, "insufficient access");
  }
  auto entry = backend_.lookup(request.modify_dn.dn);
  if (!entry) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  if (!parent.empty() && !backend_.lookup(parent)) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse,
                          ResultCode::NamingViolation, "new parent does not exist");
  }
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
  std::string diagnostic;
  const ResultCode schema = checkSchema(*entry, diagnostic);
  if (schema != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ModifyDNResponse, schema,
                          diagnostic);
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

LdapMessage Session::handleCompare(const LdapMessage &request) {
  if (request.compare.dn.empty() || request.compare.attribute.empty()) {
    return makeLdapResult(request.message_id, ProtocolOp::CompareResponse,
                          ResultCode::ProtocolError, "invalid compare request");
  }
  if (!maySearch(request.compare.dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::CompareResponse,
                          ResultCode::InsufficientAccessRights, "insufficient access");
  }
  const auto entry = backend_.lookup(request.compare.dn);
  if (!entry) {
    return makeLdapResult(request.message_id, ProtocolOp::CompareResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  const std::vector<std::string> *values = nullptr;
  for (const auto &pair : entry->attributes) {
    if (iequals(pair.first, request.compare.attribute)) {
      values = &pair.second;
      break;
    }
  }
  if (values == nullptr) {
    return makeLdapResult(request.message_id, ProtocolOp::CompareResponse,
                          ResultCode::NoSuchAttribute, "no such attribute");
  }
  bool matched = false;
  if (isUserPassword(request.compare.attribute)) {
    for (const auto &value : *values) {
      if (verifyUserPassword(value, request.compare.value)) {
        matched = true;
        break;
      }
    }
  } else {
    for (const auto &value : *values) {
      if (iequals(value, request.compare.value)) {
        matched = true;
        break;
      }
    }
  }
  return makeLdapResult(request.message_id, ProtocolOp::CompareResponse,
                        matched ? ResultCode::CompareTrue : ResultCode::CompareFalse);
}

LdapMessage Session::handleWhoAmI(const LdapMessage &request) {
  LdapMessage response =
      makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse, ResultCode::Success);
  response.extended_oid = kWhoAmIOid;
  if (!bind_dn_.empty()) {
    response.extended_value = "dn:" + bind_dn_;
  }
  return response;
}

LdapMessage Session::handlePasswordModify(const LdapMessage &request) {
  if (!bound_ || bind_dn_.empty()) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::InsufficientAccessRights, "bind required");
  }
  if (config_.require_confidentiality && !connection_.tls()) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::ConfidentialityRequired, "confidentiality required");
  }
  PasswordModifyRequest change;
  if (!decodePasswordModifyValue(request.extended_value, change)) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::ProtocolError, "invalid password modify request");
  }
  if (!change.new_password || change.new_password->empty()) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::UnwillingToPerform, "new password is required");
  }
  SimpleBindAuthenticator authenticator(backend_, config_);
  const std::string identity =
      change.user_identity.empty() ? bind_dn_ : change.user_identity;
  const auto target = authenticator.resolveName(identity);
  if (!target) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  if (!config_.root_dn.empty() && dnEquals(*target, config_.root_dn)) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::UnwillingToPerform,
                          "root_password is configured, not an entry");
  }
  const bool as_root = !config_.root_dn.empty() && dnEquals(bind_dn_, config_.root_dn);
  const bool as_self = dnEquals(bind_dn_, *target);
  if (!as_root && !as_self && !mayWrite(*target)) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::InsufficientAccessRights, "cannot change another password");
  }
  if (change.old_password &&
      authenticator.bind(*target, *change.old_password) != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::InvalidCredentials, "invalid credentials");
  }
  auto entry = backend_.lookup(*target);
  if (!entry) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::NoSuchObject, "no such object");
  }
  const ResultCode applied = applyModifications(
      *entry, {{ModifyOp::Replace, "userPassword", {encodeUserPassword(*change.new_password)}}});
  if (applied != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse, applied,
                          toString(applied));
  }
  std::string diagnostic;
  const ResultCode schema = checkSchema(*entry, diagnostic);
  if (schema != ResultCode::Success) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse, schema, diagnostic);
  }
  if (!backend_.modify(*entry)) {
    return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse,
                          ResultCode::OperationsError, "password modify failed");
  }
  backend_.persist();
  return makeLdapResult(request.message_id, ProtocolOp::ExtendedResponse, ResultCode::Success);
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
