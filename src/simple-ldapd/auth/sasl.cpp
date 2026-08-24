/**
 * @file sasl.cpp
 * @brief SASL framework stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/sasl.hpp"

#include <algorithm>

namespace simple_ldapd {

const char *toString(SaslMechanism mechanism) {
  switch (mechanism) {
  case SaslMechanism::Anonymous:
    return "ANONYMOUS";
  case SaslMechanism::Plain:
    return "PLAIN";
  case SaslMechanism::DigestMd5:
    return "DIGEST-MD5";
  case SaslMechanism::Gssapi:
    return "GSSAPI";
  case SaslMechanism::External:
    return "EXTERNAL";
  }
  return "ANONYMOUS";
}

void SaslAuthenticator::enable(SaslMechanism mechanism) {
  if (std::find(enabled_.begin(), enabled_.end(), mechanism) == enabled_.end()) {
    enabled_.push_back(mechanism);
  }
}

std::vector<std::string> SaslAuthenticator::advertised() const {
  std::vector<std::string> names;
  names.reserve(enabled_.size());
  for (auto mechanism : enabled_) {
    names.emplace_back(toString(mechanism));
  }
  return names;
}

ResultCode SaslAuthenticator::step(SaslMechanism /*mechanism*/,
                                   const std::string & /*input*/,
                                   std::string &output) const {
  output.clear();
  return ResultCode::AuthMethodNotSupported;
}

}  // namespace simple_ldapd
