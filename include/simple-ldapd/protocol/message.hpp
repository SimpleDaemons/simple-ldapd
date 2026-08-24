/**
 * @file message.hpp
 * @brief LDAPv3 message types
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/protocol/result_codes.hpp"
#include <cstdint>
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

struct LdapMessage {
  int message_id{0};
  ProtocolOp op{ProtocolOp::Unknown};
  ResultCode result{ResultCode::Unavailable};
  std::string diagnostic{"not implemented"};
};

}  // namespace simple_ldapd
