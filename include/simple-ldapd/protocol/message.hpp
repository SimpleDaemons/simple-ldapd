/**
 * @file message.hpp
 * @brief LDAPv3 message types and codec
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace simple_ldapd {

enum class ProtocolOp {
  BindRequest,
  BindResponse,
  UnbindRequest,
  SearchRequest,
  SearchResultEntry,
  SearchResultDone,
  ModifyRequest,
  ModifyResponse,
  AddRequest,
  AddResponse,
  DelRequest,
  DelResponse,
  ExtendedRequest,
  ExtendedResponse,
  Unknown
};

struct BindRequestData {
  int version{3};
  std::string dn;
  std::string password;
  bool simple{true};
};

struct SearchRequestData {
  std::string base_dn;
  SearchScope scope{SearchScope::Subtree};
  int size_limit{0};
  int time_limit{0};
  bool types_only{false};
  SearchFilter filter{SearchFilter::present("objectClass")};
  std::vector<std::string> attributes;
};

struct PartialAttribute {
  std::string type;
  std::vector<std::string> values;
};

struct SearchEntryData {
  std::string dn;
  std::vector<PartialAttribute> attributes;
};

struct LdapMessage {
  int message_id{0};
  ProtocolOp op{ProtocolOp::Unknown};
  ResultCode result{ResultCode::Success};
  std::string matched_dn;
  std::string diagnostic;
  BindRequestData bind;
  SearchRequestData search;
  SearchEntryData entry;
};

std::optional<LdapMessage> decodeLdapMessage(const std::vector<uint8_t> &wire);
std::vector<uint8_t> encodeLdapMessage(const LdapMessage &message);

LdapMessage makeBindRequest(int message_id, const std::string &dn,
                            const std::string &password);
LdapMessage makeBindResponse(int message_id, ResultCode result,
                             const std::string &diagnostic = {});
LdapMessage makeSearchRequest(int message_id, SearchRequestData search);
LdapMessage makeSearchEntry(int message_id, SearchEntryData entry);
LdapMessage makeSearchDone(int message_id, ResultCode result,
                           const std::string &diagnostic = {});
LdapMessage makeUnbindRequest(int message_id);
LdapMessage makeLdapResult(int message_id, ProtocolOp op, ResultCode result,
                           const std::string &diagnostic = {});

}  // namespace simple_ldapd
