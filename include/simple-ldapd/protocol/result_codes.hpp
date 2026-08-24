/**
 * @file result_codes.hpp
 * @brief LDAPv3 result codes (RFC 4511)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace simple_ldapd {

enum class ResultCode : uint8_t {
  Success = 0,
  OperationsError = 1,
  ProtocolError = 2,
  TimeLimitExceeded = 3,
  SizeLimitExceeded = 4,
  CompareFalse = 5,
  CompareTrue = 6,
  AuthMethodNotSupported = 7,
  StrongerAuthRequired = 8,
  Referral = 10,
  AdminLimitExceeded = 11,
  UnavailableCriticalExtension = 12,
  ConfidentialityRequired = 13,
  SaslBindInProgress = 14,
  NoSuchAttribute = 16,
  UndefinedAttributeType = 17,
  InappropriateMatching = 18,
  ConstraintViolation = 19,
  AttributeOrValueExists = 20,
  InvalidAttributeSyntax = 21,
  NoSuchObject = 32,
  AliasProblem = 33,
  InvalidDnSyntax = 34,
  AliasDereferencingProblem = 36,
  InappropriateAuthentication = 48,
  InvalidCredentials = 49,
  InsufficientAccessRights = 50,
  Busy = 51,
  Unavailable = 52,
  UnwillingToPerform = 53,
  LoopDetect = 54,
  NamingViolation = 64,
  ObjectClassViolation = 65,
  NotAllowedOnNonLeaf = 66,
  NotAllowedOnRdn = 67,
  EntryAlreadyExists = 68,
  ObjectClassModsProhibited = 69,
  AffectsMultipleDsas = 71,
  Other = 80
};

inline const char *toString(ResultCode code) {
  switch (code) {
  case ResultCode::Success:
    return "success";
  case ResultCode::OperationsError:
    return "operationsError";
  case ResultCode::ProtocolError:
    return "protocolError";
  case ResultCode::TimeLimitExceeded:
    return "timeLimitExceeded";
  case ResultCode::SizeLimitExceeded:
    return "sizeLimitExceeded";
  case ResultCode::CompareFalse:
    return "compareFalse";
  case ResultCode::CompareTrue:
    return "compareTrue";
  case ResultCode::UnavailableCriticalExtension:
    return "unavailableCriticalExtension";
  case ResultCode::Busy:
    return "busy";
  case ResultCode::Unavailable:
    return "unavailable";
  case ResultCode::UnwillingToPerform:
    return "unwillingToPerform";
  case ResultCode::InvalidCredentials:
    return "invalidCredentials";
  case ResultCode::NoSuchObject:
    return "noSuchObject";
  case ResultCode::AuthMethodNotSupported:
    return "authMethodNotSupported";
  case ResultCode::SaslBindInProgress:
    return "saslBindInProgress";
  case ResultCode::NoSuchAttribute:
    return "noSuchAttribute";
  case ResultCode::InvalidDnSyntax:
    return "invalidDNSyntax";
  case ResultCode::InsufficientAccessRights:
    return "insufficientAccessRights";
  case ResultCode::EntryAlreadyExists:
    return "entryAlreadyExists";
  case ResultCode::NamingViolation:
    return "namingViolation";
  case ResultCode::NotAllowedOnNonLeaf:
    return "notAllowedOnNonLeaf";
  case ResultCode::AttributeOrValueExists:
    return "attributeOrValueExists";
  case ResultCode::ConfidentialityRequired:
    return "confidentialityRequired";
  case ResultCode::UndefinedAttributeType:
    return "undefinedAttributeType";
  case ResultCode::ConstraintViolation:
    return "constraintViolation";
  case ResultCode::InvalidAttributeSyntax:
    return "invalidAttributeSyntax";
  case ResultCode::ObjectClassViolation:
    return "objectClassViolation";
  default:
    return "other";
  }
}

}  // namespace simple_ldapd
