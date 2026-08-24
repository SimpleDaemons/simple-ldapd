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
  ModifyDNRequest,
  ModifyDNResponse,
  ExtendedRequest,
  ExtendedResponse,
  Unknown
};

struct BindRequestData {
  int version{3};
  std::string dn;
  std::string password;
  bool simple{true};
  std::string sasl_mechanism;
  std::string sasl_credentials;
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

struct AddRequestData {
  std::string dn;
  std::vector<PartialAttribute> attributes;
};

struct ModifyRequestData {
  std::string dn;
  std::vector<AttributeModification> changes;
};

struct ModifyDnRequestData {
  std::string dn;
  std::string new_rdn;
  bool delete_old_rdn{true};
  std::string new_superior;
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
  AddRequestData add;
  ModifyRequestData modify;
  std::string delete_dn;
  ModifyDnRequestData modify_dn;
  std::string extended_oid;
  std::string extended_value;
  std::string server_sasl_creds;
};

std::optional<LdapMessage> decodeLdapMessage(const std::vector<uint8_t> &wire);
std::vector<uint8_t> encodeLdapMessage(const LdapMessage &message);

LdapMessage makeBindRequest(int message_id, const std::string &dn,
                            const std::string &password);
LdapMessage makeSaslBindRequest(int message_id, const std::string &dn,
                                const std::string &mechanism,
                                const std::string &credentials = {});
LdapMessage makeBindResponse(int message_id, ResultCode result,
                             const std::string &diagnostic = {},
                             const std::string &server_sasl_creds = {});
LdapMessage makeSearchRequest(int message_id, SearchRequestData search);
LdapMessage makeSearchEntry(int message_id, SearchEntryData entry);
LdapMessage makeSearchDone(int message_id, ResultCode result,
                           const std::string &diagnostic = {});
LdapMessage makeUnbindRequest(int message_id);
LdapMessage makeAddRequest(int message_id, AddRequestData add);
LdapMessage makeModifyRequest(int message_id, ModifyRequestData modify);
LdapMessage makeDelRequest(int message_id, const std::string &dn);
LdapMessage makeModifyDnRequest(int message_id, ModifyDnRequestData modify_dn);
LdapMessage makeExtendedRequest(int message_id, const std::string &oid,
                                const std::string &value = {});
LdapMessage makeLdapResult(int message_id, ProtocolOp op, ResultCode result,
                           const std::string &diagnostic = {});

inline constexpr const char *kPasswordModifyOid = "1.3.6.1.4.1.4203.1.11.1";

struct PasswordModifyRequest {
  std::string user_identity;
  std::optional<std::string> old_password;
  std::optional<std::string> new_password;
};

std::string encodePasswordModifyValue(const PasswordModifyRequest &request);
bool decodePasswordModifyValue(const std::string &value, PasswordModifyRequest &request);
LdapMessage makePasswordModifyRequest(int message_id, const PasswordModifyRequest &request);

DirectoryEntry toDirectoryEntry(const AddRequestData &add);
AddRequestData toAddRequest(const DirectoryEntry &entry);

}  // namespace simple_ldapd
