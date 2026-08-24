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
  inner.writeOctetString(kBerSimpleAuth, bind.password);
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
  return reader.skip();
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

std::vector<uint8_t> encodeSearchEntry(const SearchEntryData &entry) {
  BerWriter attrs;
  for (const auto &attribute : entry.attributes) {
    BerWriter values;
    for (const auto &value : attribute.values) {
      values.writeOctetString(value);
    }
    BerWriter partial;
    partial.writeOctetString(attribute.type);
    partial.writeConstructed(kBerSet, values.bytes());
    attrs.writeConstructed(kBerSequence, partial.bytes());
  }
  BerWriter inner;
  inner.writeOctetString(entry.dn);
  inner.writeConstructed(kBerSequence, attrs.bytes());
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
    entry.attributes.push_back(attribute);
  }
  return attrs.ok();
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
    if (!body.skip()) {
      return std::nullopt;
    }
    break;
  case kBerAddRequest:
    message.op = ProtocolOp::AddRequest;
    if (!body.skip()) {
      return std::nullopt;
    }
    break;
  case kBerDelRequest:
    message.op = ProtocolOp::DelRequest;
    if (!body.skip()) {
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
  return message;
}

std::vector<uint8_t> encodeLdapMessage(const LdapMessage &message) {
  auto resultOp = [&](uint8_t tag) {
    BerWriter writer;
    writer.writeConstructed(
        tag, encodeResult(message.result, message.matched_dn, message.diagnostic));
    return writer.take();
  };

  std::vector<uint8_t> protocol_op;
  switch (message.op) {
  case ProtocolOp::BindRequest:
    protocol_op = encodeBindRequest(message.bind);
    break;
  case ProtocolOp::BindResponse:
    protocol_op = resultOp(kBerBindResponse);
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
  case ProtocolOp::ModifyResponse:
    protocol_op = resultOp(kBerModifyResponse);
    break;
  case ProtocolOp::AddResponse:
    protocol_op = resultOp(kBerAddResponse);
    break;
  case ProtocolOp::DelResponse:
    protocol_op = resultOp(kBerDelResponse);
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

LdapMessage makeBindResponse(int message_id, ResultCode result,
                             const std::string &diagnostic) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = ProtocolOp::BindResponse;
  message.result = result;
  message.diagnostic = diagnostic;
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

LdapMessage makeLdapResult(int message_id, ProtocolOp op, ResultCode result,
                           const std::string &diagnostic) {
  LdapMessage message;
  message.message_id = message_id;
  message.op = op;
  message.result = result;
  message.diagnostic = diagnostic;
  return message;
}

}  // namespace simple_ldapd
