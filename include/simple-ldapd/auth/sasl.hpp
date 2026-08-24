/**
 * @file sasl.hpp
 * @brief SASL mechanism framework stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/protocol/result_codes.hpp"
#include <string>
#include <vector>

namespace simple_ldapd {

enum class SaslMechanism { Anonymous, Plain, DigestMd5, Gssapi, External };

const char *toString(SaslMechanism mechanism);

class SaslAuthenticator {
public:
  void enable(SaslMechanism mechanism);
  std::vector<std::string> advertised() const;
  ResultCode step(SaslMechanism mechanism, const std::string &input,
                  std::string &output) const;

private:
  std::vector<SaslMechanism> enabled_;
};

}  // namespace simple_ldapd
