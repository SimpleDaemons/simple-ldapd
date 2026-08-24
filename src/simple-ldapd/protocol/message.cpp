/**
 * @file message.cpp
 * @brief LDAPv3 message codec
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/protocol/message.hpp"

#include "simple-ldapd/protocol/ber.hpp"

namespace simple_ldapd {

namespace {

std::vector<uint8_t> encodeResult(ResultCode result, const std::string &matched_dn,
                                  const std::string &diagnostic) {
  BerWriter writer;
  writer.writeEnumerated(static_cast<int>(result));
  writer.writeOctetString(matched_dn);
  writer.writeOctetString(diagnostic);
  return writer.take();
}

bool decodeResult(BerReader &reader, LdapMessage &message) {
  int code = 0;
  if (!reader.readEnumerated(code) || !reader.readOctetString(message.matched_dn) ||
      !reader.readOctetString(message.diagnostic)) {
    return false;
  }
  message.result = static_cast<ResultCode>(code);
  while (!reader.atEnd()) {
    if (reader.peekTag() == kBerServerSaslCreds) {
      if (!reader.readOctetString(kBerServerSaslCreds, message.server_sasl_creds)) {
        return false;
      }
      continue;
    }
    if (reader.peekTag() == kBerContext10) {
      if (!reader.readOctetString(kBerContext10, message.extended_oid)) {
        return false;
      }
      continue;
    }
    if (reader.peekTag() == kBerContext11) {
      if (!reader.readOctetString(kBerContext11, message.extended_value)) {
        return false;
      }
      continue;
    }
    if (!reader.skip()) {
      return false;
    }
  }
  return reader.ok();
}

std::vector<uint8_t> encodeBindRequest(const BindRequestData &bind) {
  BerWriter inner;
  inner.writeInteger(bind.version);
  inner.writeOctetString(bind.dn);
  if (bind.simple) {
    inner.writeOctetString(kBerSimpleAuth, bind.password);
  } else {
    BerWriter sasl;
    sasl.writeOctetString(bind.sasl_mechanism);
    if (!bind.sasl_credentials.empty()) {
      sasl.writeOctetString(bind.sasl_credentials);
    }
    inner.writeConstructed(kBerSaslAuth, sasl.bytes());
  }
  BerWriter outer;
  outer.writeConstructed(kBerBindRequest, inner.bytes());
  return outer.take();
}

bool decodeBindRequest(BerReader &reader, BindRequestData &bind) {
  int64_t version = 0;
  if (!reader.readInteger(version) || !reader.readOctetString(bind.dn)) {
    return false;
  }
  bind.version = static_cast<int>(version);
  bind.simple = reader.peekTag() == kBerSimpleAuth;
  if (bind.simple) {
    return reader.readOctetString(kBerSimpleAuth, bind.password);
  }
  if (reader.peekTag() != kBerSaslAuth) {
    return reader.skip();
  }
  BerReader sasl;
  if (!reader.readConstructed(kBerSaslAuth, sasl) || !sasl.readOctetString(bind.sasl_mechanism)) {
    return false;
  }
  if (!sasl.atEnd() && !sasl.readOctetString(bind.sasl_credentials)) {
    return false;
  }
  return sasl.ok();
}

std::vector<uint8_t> encodeBindResponse(const LdapMessage &message) {
  BerWriter inner;
  inner.writeEnumerated(static_cast<int>(message.result));
  inner.writeOctetString(message.matched_dn);
  inner.writeOctetString(message.diagnostic);
  if (!message.server_sasl_creds.empty()) {
    inner.writeOctetString(kBerServerSaslCreds, message.server_sasl_creds);
  }
  BerWriter outer;
  outer.writeConstructed(kBerBindResponse, inner.bytes());
  return outer.take();
}

std::vector<uint8_t> encodeSearchRequest(const SearchRequestData &search) {
  BerWriter inner;
  inner.writeOctetString(search.base_dn);
  inner.writeEnumerated(static_cast<int>(search.scope));
  inner.writeEnumerated(0);  // neverDerefAliases
  inner.writeInteger(search.size_limit);
  inner.writeInteger(search.time_limit);
  inner.writeBoolean(search.types_only);
  const auto filter = search.filter.encodeBer();
  inner.writeBytes(filter.data(), filter.size());
  BerWriter attrs;
  for (const auto &name : search.attributes) {
    attrs.writeOctetString(name);
  }
  inner.writeConstructed(kBerSequence, attrs.bytes());
  BerWriter outer;
  outer.writeConstructed(kBerSearchRequest, inner.bytes());
  return outer.take();
}

bool decodeSearchRequest(BerReader &reader, SearchRequestData &search) {
  int64_t size_limit = 0;
  int64_t time_limit = 0;
  int scope = 0;
  int deref = 0;
  if (!reader.readOctetString(search.base_dn) || !reader.readEnumerated(scope) ||
      !reader.readEnumerated(deref) || !reader.readInteger(size_limit) ||
      !reader.readInteger(time_limit) || !reader.readBoolean(search.types_only)) {
    return false;
  }
  search.scope = static_cast<SearchScope>(scope);
  search.size_limit = static_cast<int>(size_limit);
  search.time_limit = static_cast<int>(time_limit);
  if (!SearchFilter::decodeBer(reader, search.filter)) {
    return false;
  }
  BerReader attrs;
  if (!reader.readConstructed(kBerSequence, attrs)) {
    return false;
  }
  while (!attrs.atEnd()) {
    std::string name;
    if (!attrs.readOctetString(name)) {
      return false;
    }
    search.attributes.push_back(name);
  }
  return attrs.ok();
}

std::vector<uint8_t> encodeAttributes(const std::vector<PartialAttribute> &attributes) {
  BerWriter attrs;
  for (const auto &attribute : attributes) {
    BerWriter values;
    for (const auto &value : attribute.values) {
      values.writeOctetString(value);
    }
    BerWriter partial;
    partial.writeOctetString(attribute.type);
    partial.writeConstructed(kBerSet, values.bytes());
    attrs.writeConstructed(kBerSequence, partial.bytes());
  }
  return attrs.take();
}

bool decodeAttributes(BerReader &attrs, std::vector<PartialAttribute> &out) {
  while (!attrs.atEnd()) {
    BerReader partial;
    if (!attrs.readConstructed(kBerSequence, partial)) {
      return false;
    }
    PartialAttribute attribute;
    if (!partial.readOctetString(attribute.type)) {
      return false;
    }
    BerReader values;
    if (!partial.readConstructed(kBerSet, values)) {
      return false;
    }
    while (!values.atEnd()) {
      std::string value;
      if (!values.readOctetString(value)) {
        return false;
      }
      attribute.values.push_back(value);
    }
    out.push_back(attribute);
  }
  return attrs.ok();
}

std::vector<uint8_t> encodeSearchEntry(const SearchEntryData &entry) {
  BerWriter inner;
  inner.writeOctetString(entry.dn);
  inner.writeConstructed(kBerSequence, encodeAttributes(entry.attributes));
  BerWriter outer;
  outer.writeConstructed(kBerSearchResultEntry, inner.bytes());
  return outer.take();
}

bool decodeSearchEntry(BerReader &reader, SearchEntryData &entry) {
  if (!reader.readOctetString(entry.dn)) {
    return false;
  }
  BerReader attrs;
  if (!reader.readConstructed(kBerSequence, attrs)) {
    return false;
  }
  return decodeAttributes(attrs, entry.attributes);
}

std::vector<uint8_t> encodeAddRequest(const AddRequestData &add) {
  BerWriter inner;
  inner.writeOctetString(add.dn);
  inner.writeConstructed(kBerSequence, encodeAttributes(add.attributes));
  BerWriter outer;
  outer.writeConstructed(kBerAddRequest, inner.bytes());
  return outer.take();
}

bool decodeAddRequest(BerReader &reader, AddRequestData &add) {
  if (!reader.readOctetString(add.dn)) {
    return false;
  }
  BerReader attrs;
  if (!reader.readConstructed(kBerSequence, attrs)) {
    return false;
  }
  return decodeAttributes(attrs, add.attributes);
}

std::vector<uint8_t> encodeModifyRequest(const ModifyRequestData &modify) {
  BerWriter changes;
  for (const auto &change : modify.changes) {
    BerWriter values;
    for (const auto &value : change.values) {
      values.writeOctetString(value);
    }
    BerWriter partial;
    partial.writeOctetString(change.type);
    partial.writeConstructed(kBerSet, values.bytes());
    BerWriter item;
    item.writeEnumerated(static_cast<int>(change.op));
    item.writeConstructed(kBerSequence, partial.bytes());
    changes.writeConstructed(kBerSequence, item.bytes());
  }
  BerWriter inner;
  inner.writeOctetString(modify.dn);
  inner.writeConstructed(kBerSequence, changes.bytes());
  BerWriter outer;
  outer.writeConstructed(kBerModifyRequest, inner.bytes());
  return outer.take();
}

bool decodeModifyRequest(BerReader &reader, ModifyRequestData &modify) {
  if (!reader.readOctetString(modify.dn)) {
    return false;
  }
  BerReader changes;
  if (!reader.readConstructed(kBerSequence, changes)) {
    return false;
  }
  while (!changes.atEnd()) {
    BerReader item;
    if (!changes.readConstructed(kBerSequence, item)) {
      return false;
    }
    int op = 0;
    if (!item.readEnumerated(op)) {
      return false;
    }
    BerReader partial;
    if (!item.readConstructed(kBerSequence, partial)) {
      return false;
    }
    AttributeModification change;
    change.op = static_cast<ModifyOp>(op);
    if (!partial.readOctetString(change.type)) {
      return false;
    }
    BerReader values;
    if (!partial.readConstructed(kBerSet, values)) {
      return false;
    }
    while (!values.atEnd()) {
      std::string value;
      if (!values.readOctetString(value)) {
        return false;
      }
      change.values.push_back(value);
    }
    modify.changes.push_back(std::move(change));
  }
  return changes.ok();
}

std::vector<uint8_t> encodeModifyDnRequest(const ModifyDnRequestData &modify_dn) {
  BerWriter inner;
  inner.writeOctetString(modify_dn.dn);
  inner.writeOctetString(modify_dn.new_rdn);
  inner.writeBoolean(modify_dn.delete_old_rdn);
  if (!modify_dn.new_superior.empty()) {
    inner.writeOctetString(kBerSimpleAuth, modify_dn.new_superior);
  }
  BerWriter outer;
  outer.writeConstructed(kBerModifyDNRequest, inner.bytes());
  return outer.take();
}

bool decodeModifyDnRequest(BerReader &reader, ModifyDnRequestData &modify_dn) {
  if (!reader.readOctetString(modify_dn.dn) || !reader.readOctetString(modify_dn.new_rdn) ||
      !reader.readBoolean(modify_dn.delete_old_rdn)) {
    return false;
  }
  if (!reader.atEnd() && reader.peekTag() == kBerSimpleAuth) {
    return reader.readOctetString(kBerSimpleAuth, modify_dn.new_superior);
  }
  return reader.ok();
}

std::vector<uint8_t> encodeCompareRequest(const CompareRequestData &compare) {
  BerWriter ava;
  ava.writeOctetString(compare.attribute);
  ava.writeOctetString(compare.value);
  BerWriter inner;
  inner.writeOctetString(compare.dn);
  inner.writeConstructed(kBerSequence, ava.bytes());
  BerWriter outer;
  outer.writeConstructed(kBerCompareRequest, inner.bytes());
  return outer.take();
}

bool decodeCompareRequest(BerReader &reader, CompareRequestData &compare) {
  BerReader ava;
  return reader.readOctetString(compare.dn) && reader.readConstructed(kBerSequence, ava) &&
         ava.readOctetString(compare.attribute) && ava.readOctetString(compare.value) &&
         reader.ok();
}

std::vector<uint8_t> encodeControls(const std::vector<LdapControl> &controls) {
  BerWriter list;
  for (const auto &control : controls) {
    BerWriter item;
    item.writeOctetString(control.oid);
    if (control.critical) {
      item.writeBoolean(true);
    }
    if (!control.value.empty()) {
      item.writeOctetString(control.value);
    }
    list.writeConstructed(kBerSequence, item.bytes());
  }
  BerWriter outer;
  outer.writeConstructed(kBerControls, list.bytes());
  return outer.take();
}

bool decodeControls(BerReader &reader, std::vector<LdapControl> &controls) {
  while (!reader.atEnd()) {
    BerReader item;
    if (!reader.readConstructed(kBerSequence, item)) {
      return false;
    }
    LdapControl control;
    if (!item.readOctetString(control.oid)) {
      return false;
    }
    if (!item.atEnd() && item.peekTag() == kBerBoolean) {
      if (!item.readBoolean(control.critical)) {
        return false;
      }
    }
    if (!item.atEnd()) {
      if (!item.readOctetString(control.value)) {
        return false;
      }
    }
    controls.push_back(std::move(control));
  }
  return reader.ok();
}

std::vector<uint8_t> encodeExtendedRequest(const std::string &oid, const std::string &value) {
  BerWriter inner;
  inner.writeOctetString(kBerSimpleAuth, oid);
  if (!value.empty()) {
    inner.writeOctetString(kBerContext1, value);
  }
  BerWriter outer;
  outer.writeConstructed(kBerExtendedRequest, inner.bytes());
  return outer.take();
}

bool decodeExtendedRequest(BerReader &reader, std::string &oid, std::string &value) {
  if (!reader.readOctetString(kBerSimpleAuth, oid)) {
    return false;
  }
  if (!reader.atEnd() && reader.peekTag() == kBerContext1) {
    return reader.readOctetString(kBerContext1, value);
  }
  return reader.ok();
}

}  // namespace

std::optional<LdapMessage> decodeLdapMessage(const std::vector<uint8_t> &wire) {
  BerReader top(wire);
  BerReader body;
  if (!top.readConstructed(kBerSequence, body)) {
    return std::nullopt;
  }
  int64_t message_id = 0;
  if (!body.readInteger(message_id)) {
    return std::nullopt;
  }
  LdapMessage message;
  message.message_id = static_cast<int>(message_id);
  const uint8_t tag = body.peekTag();
  BerReader op;
  switch (tag) {
  case kBerBindRequest:
    message.op = ProtocolOp::BindRequest;
    if (!body.readConstructed(tag, op) || !decodeBindRequest(op, message.bind)) {
      return std::nullopt;
    }
    break;
  case kBerBindResponse:
    message.op = ProtocolOp::BindResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerUnbindRequest:
    message.op = ProtocolOp::UnbindRequest;
    if (!body.skip()) {
      return std::nullopt;
    }
    break;
  case kBerSearchRequest:
    message.op = ProtocolOp::SearchRequest;
    if (!body.readConstructed(tag, op) || !decodeSearchRequest(op, message.search)) {
      return std::nullopt;
    }
    break;
  case kBerSearchResultEntry:
    message.op = ProtocolOp::SearchResultEntry;
    if (!body.readConstructed(tag, op) || !decodeSearchEntry(op, message.entry)) {
      return std::nullopt;
    }
    break;
  case kBerSearchResultDone:
    message.op = ProtocolOp::SearchResultDone;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerModifyRequest:
    message.op = ProtocolOp::ModifyRequest;
    if (!body.readConstructed(tag, op) || !decodeModifyRequest(op, message.modify)) {
      return std::nullopt;
    }
    break;
  case kBerModifyResponse:
    message.op = ProtocolOp::ModifyResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerAddRequest:
    message.op = ProtocolOp::AddRequest;
    if (!body.readConstructed(tag, op) || !decodeAddRequest(op, message.add)) {
      return std::nullopt;
    }
    break;
  case kBerAddResponse:
    message.op = ProtocolOp::AddResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerDelRequest:
    message.op = ProtocolOp::DelRequest;
    if (!body.readOctetString(kBerDelRequest, message.delete_dn)) {
      return std::nullopt;
    }
    break;
  case kBerDelResponse:
    message.op = ProtocolOp::DelResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerModifyDNRequest:
    message.op = ProtocolOp::ModifyDNRequest;
    if (!body.readConstructed(tag, op) || !decodeModifyDnRequest(op, message.modify_dn)) {
      return std::nullopt;
    }
    break;
  case kBerModifyDNResponse:
    message.op = ProtocolOp::ModifyDNResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerCompareRequest:
    message.op = ProtocolOp::CompareRequest;
    if (!body.readConstructed(tag, op) || !decodeCompareRequest(op, message.compare)) {
      return std::nullopt;
    }
    break;
  case kBerCompareResponse:
    message.op = ProtocolOp::CompareResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  case kBerExtendedRequest:
    message.op = ProtocolOp::ExtendedRequest;
    if (!body.readConstructed(tag, op) ||
        !decodeExtendedRequest(op, message.extended_oid, message.extended_value)) {
      return std::nullopt;
    }
    break;
  case kBerExtendedResponse:
    message.op = ProtocolOp::ExtendedResponse;
    if (!body.readConstructed(tag, op) || !decodeResult(op, message)) {
      return std::nullopt;
    }
    break;
  default:
    message.op = ProtocolOp::Unknown;
    if (!body.skip()) {
      return std::nullopt;
    }
    break;
  }
  if (!body.atEnd() && body.peekTag() == kBerControls) {
    BerReader controls;
    if (!body.readConstructed(kBerControls, controls) ||
        !decodeControls(controls, message.controls)) {
      return std::nullopt;
    }
  }
  return message;
}

std::vector<uint8_t> encodeLdapMessage(const LdapMessage &message) {
  auto resultOp = [&](uint8_t tag) {
    auto inner_bytes =
        encodeResult(message.result, message.matched_dn, message.diagnostic);
    if (tag == kBerExtendedResponse &&
        (!message.extended_oid.empty() || !message.extended_value.empty())) {
      BerWriter extra;
      extra.writeBytes(inner_bytes.data(), inner_bytes.size());
      if (!message.extended_oid.empty()) {
        extra.writeOctetString(kBerContext10, message.extended_oid);
      }
      extra.writeOctetString(kBerContext11, message.extended_value);
      inner_bytes = extra.take();
    }
    BerWriter writer;
    writer.writeConstructed(tag, inner_bytes);
    return writer.take();
  };

  std::vector<uint8_t> protocol_op;
  switch (message.op) {
  case ProtocolOp::BindRequest:
    protocol_op = encodeBindRequest(message.bind);
    break;
  case ProtocolOp::BindResponse:
    protocol_op = encodeBindResponse(message);
    break;
  case ProtocolOp::UnbindRequest: {
    BerWriter writer;
    writer.writeNull(kBerUnbindRequest);
    protocol_op = writer.take();
    break;
  }
  case ProtocolOp::SearchRequest:
    protocol_op = encodeSearchRequest(message.search);
    break;
  case ProtocolOp::SearchResultEntry:
    protocol_op = encodeSearchEntry(message.entry);
    break;
  case ProtocolOp::SearchResultDone:
    protocol_op = resultOp(kBerSearchResultDone);
    break;
  case ProtocolOp::ModifyRequest:
    protocol_op = encodeModifyRequest(message.modify);
    break;
  case ProtocolOp::ModifyResponse:
    protocol_op = resultOp(kBerModifyResponse);
    break;
  case ProtocolOp::AddRequest:
    protocol_op = encodeAddRequest(message.add);
    break;
  case ProtocolOp::AddResponse:
    protocol_op = resultOp(kBerAddResponse);
    break;
  case ProtocolOp::DelRequest: {
    BerWriter writer;
    writer.writeOctetString(kBerDelRequest, message.delete_dn);
    protocol_op = writer.take();
    break;
  }
  case ProtocolOp::DelResponse:
    protocol_op = resultOp(kBerDelResponse);
    break;
  case ProtocolOp::ModifyDNRequest:
    protocol_op = encodeModifyDnRequest(message.modify_dn);
    break;
  case ProtocolOp::ModifyDNResponse:
    protocol_op = resultOp(kBerModifyDNResponse);
    break;
  case ProtocolOp::CompareRequest:
    protocol_op = encodeCompareRequest(message.compare);
    break;
  case ProtocolOp::CompareResponse:
    protocol_op = resultOp(kBerCompareResponse);
    break;
  case ProtocolOp::ExtendedRequest:
    protocol_op = encodeExtendedRequest(message.extended_oid, message.extended_value);
    break;
  case ProtocolOp::ExtendedResponse:
    protocol_op = resultOp(kBerExtendedResponse);
    break;
  default: {
    protocol_op = resultOp(kBerSearchResultDone);
    break;
  }
  }
  BerWriter body;
  body.writeInteger(message.message_id);
  body.writeBytes(protocol_op.data(), protocol_op.size());
  if (!message.controls.empty()) {
    const auto controls = encodeControls(message.controls);
    body.writeBytes(controls.data(), controls.size());
  }
  BerWriter envelope;
  envelope.writeConstructed(kBerSequence, body.bytes());
  return envelope.take();
}

LdapMessage makeBindRequest(int message_id, const std::string &dn,
                            const std::string &password) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::BindRequest;
  message.bind.dn = dn;
  message.bind.password = password;
  return message;
}

LdapMessage makeSaslBindRequest(int message_id, const std::string &dn,
                                const std::string &mechanism,
                                const std::string &credentials) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::BindRequest;
  message.bind.dn = dn;
  message.bind.simple = false;
  message.bind.sasl_mechanism = mechanism;
  message.bind.sasl_credentials = credentials;
  return message;
}

LdapMessage makeBindResponse(int message_id, ResultCode result,
                             const std::string &diagnostic,
                             const std::string &server_sasl_creds) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::BindResponse;
  message.result = result;
  message.diagnostic = diagnostic;
  message.server_sasl_creds = server_sasl_creds;
  return message;
}

LdapMessage makeSearchRequest(int message_id, SearchRequestData search) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::SearchRequest;
  message.search = std::move(search);
  return message;
}

LdapMessage makeSearchEntry(int message_id, SearchEntryData entry) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::SearchResultEntry;
  message.entry = std::move(entry);
  return message;
}

LdapMessage makeSearchDone(int message_id, ResultCode result,
                           const std::string &diagnostic) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::SearchResultDone;
  message.result = result;
  message.diagnostic = diagnostic;
  return message;
}

LdapMessage makeUnbindRequest(int message_id) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::UnbindRequest;
  return message;
}

LdapMessage makeAddRequest(int message_id, AddRequestData add) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::AddRequest;
  message.add = std::move(add);
  return message;
}

LdapMessage makeModifyRequest(int message_id, ModifyRequestData modify) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::ModifyRequest;
  message.modify = std::move(modify);
  return message;
}

LdapMessage makeDelRequest(int message_id, const std::string &dn) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::DelRequest;
  message.delete_dn = dn;
  return message;
}

LdapMessage makeModifyDnRequest(int message_id, ModifyDnRequestData modify_dn) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::ModifyDNRequest;
  message.modify_dn = std::move(modify_dn);
  return message;
}

LdapMessage makeCompareRequest(int message_id, CompareRequestData compare) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::CompareRequest;
  message.compare = std::move(compare);
  return message;
}

std::string encodePagedResultsValue(int size, const std::string &cookie) {
  BerWriter inner;
  inner.writeInteger(size);
  inner.writeOctetString(cookie);
  BerWriter outer;
  outer.writeConstructed(kBerSequence, inner.bytes());
  return std::string(outer.bytes().begin(), outer.bytes().end());
}

bool decodePagedResultsValue(const std::string &value, int &size, std::string &cookie) {
  const std::vector<uint8_t> wire(value.begin(), value.end());
  BerReader reader(wire);
  BerReader inner;
  int64_t parsed = 0;
  if (!reader.readConstructed(kBerSequence, inner) || !inner.readInteger(parsed) ||
      !inner.readOctetString(cookie)) {
    return false;
  }
  size = static_cast<int>(parsed);
  return reader.ok();
}

LdapControl makePagedResultsControl(int size, const std::string &cookie, bool critical) {
  LdapControl control;
  control.oid = kPagedResultsOid;
  control.critical = critical;
  control.value = encodePagedResultsValue(size, cookie);
  return control;
}

LdapMessage makeExtendedRequest(int message_id, const std::string &oid,
                                const std::string &value) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::ExtendedRequest;
  message.extended_oid = oid;
  message.extended_value = value;
  return message;
}

LdapMessage makeLdapResult(int message_id, ProtocolOp op, ResultCode result,
                           const std::string &diagnostic) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = op;
  message.result = result;
  message.diagnostic = diagnostic;
  return message;
}

std::string encodePasswordModifyValue(const PasswordModifyRequest &request) {
  BerWriter fields;
  if (!request.user_identity.empty()) {
    fields.writeOctetString(kBerSimpleAuth, request.user_identity);
  }
  if (request.old_password) {
    fields.writeOctetString(kBerContext1, *request.old_password);
  }
  if (request.new_password) {
    fields.writeOctetString(kBerContext2, *request.new_password);
  }
  BerWriter sequence;
  sequence.writeConstructed(kBerSequence, fields.bytes());
  const auto bytes = sequence.take();
  return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

bool decodePasswordModifyValue(const std::string &value, PasswordModifyRequest &request) {
  request = {};
  if (value.empty()) {
    return true;
  }
  const std::vector<uint8_t> wire(value.begin(), value.end());
  BerReader top(wire);
  BerReader sequence;
  if (!top.readConstructed(kBerSequence, sequence)) {
    return false;
  }
  while (!sequence.atEnd()) {
    const uint8_t tag = sequence.peekTag();
    std::string field;
    if (tag == kBerSimpleAuth) {
      if (!sequence.readOctetString(kBerSimpleAuth, field)) {
        return false;
      }
      request.user_identity = std::move(field);
    } else if (tag == kBerContext1) {
      if (!sequence.readOctetString(kBerContext1, field)) {
        return false;
      }
      request.old_password = std::move(field);
    } else if (tag == kBerContext2) {
      if (!sequence.readOctetString(kBerContext2, field)) {
        return false;
      }
      request.new_password = std::move(field);
    } else if (!sequence.skip()) {
      return false;
    }
  }
  return sequence.ok();
}

LdapMessage makePasswordModifyRequest(int message_id, const PasswordModifyRequest &request) {
  return makeExtendedRequest(message_id, kPasswordModifyOid, encodePasswordModifyValue(request));
}

DirectoryEntry toDirectoryEntry(const AddRequestData &add) {
  DirectoryEntry entry;
  entry.dn = add.dn;
  for (const auto &attribute : add.attributes) {
    entry.attributes[attribute.type] = attribute.values;
  }
  return entry;
}

AddRequestData toAddRequest(const DirectoryEntry &entry) {
  AddRequestData add;
  add.dn = entry.dn;
  for (const auto &pair : entry.attributes) {
    add.attributes.push_back(PartialAttribute{pair.first, pair.second});
  }
  return add;
}

}  // namespace simple_ldapd
