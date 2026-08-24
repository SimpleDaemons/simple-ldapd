/**
 * @file sasl.hpp
 * @brief SASL bind mechanisms
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include <string>
#include <vector>

namespace simple_ldapd {

enum class SaslMechanism { Anonymous, Plain, DigestMd5, Gssapi, External };

const char *toString(SaslMechanism mechanism);
bool parseSaslMechanism(const std::string &name, SaslMechanism &mechanism);
std::string saslPlainCredentials(const std::string &authzid, const std::string &authcid,
                                 const std::string &password);
bool saslDigestClientResponse(const std::string &challenge, const std::string &username,
                              const std::string &password, const std::string &digest_uri,
                              std::string &credentials, std::string &error);

struct SaslBindResult {
  ResultCode result{ResultCode::AuthMethodNotSupported};
  std::string bind_dn;
  std::string server_creds;
  std::string diagnostic;
};

class SaslAuthenticator {
public:
  void enable(SaslMechanism mechanism);
  bool supports(SaslMechanism mechanism) const;
  std::vector<std::string> advertised() const;
  ResultCode step(SaslMechanism mechanism, const std::string &input,
                  std::string &output) const;
  SaslBindResult bind(Backend &backend, const LdapConfig &config,
                      const BindRequestData &request, bool tls,
                      std::string &digest_nonce) const;

private:
  std::vector<SaslMechanism> enabled_;
};

}  // namespace simple_ldapd
