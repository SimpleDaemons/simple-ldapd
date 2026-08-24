/**
 * @file sasl.cpp
 * @brief SASL PLAIN, DIGEST-MD5, EXTERNAL, and GSSAPI hook
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/sasl.hpp"

#include "simple-ldapd/auth/bind.hpp"
#include "simple-ldapd/utils/dn.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

#ifdef SIMPLE_LDAPD_SSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

namespace simple_ldapd {

namespace {

std::string toHex(const unsigned char *data, size_t size) {
  static const char *kDigits = "0123456789abcdef";
  std::string out;
  out.resize(size * 2);
  for (size_t i = 0; i < size; ++i) {
    out[i * 2] = kDigits[data[i] >> 4];
    out[i * 2 + 1] = kDigits[data[i] & 0x0f];
  }
  return out;
}

#ifdef SIMPLE_LDAPD_SSL
std::string md5Raw(const std::string &data) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int length = 0;
  EVP_Digest(data.data(), data.size(), digest, &length, EVP_md5(), nullptr);
  return std::string(reinterpret_cast<char *>(digest), length);
}

std::string md5Hex(const std::string &data) {
  const std::string raw = md5Raw(data);
  return toHex(reinterpret_cast<const unsigned char *>(raw.data()), raw.size());
}

std::string randomNonce() {
  unsigned char bytes[16];
  if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
    return {};
  }
  return toHex(bytes, sizeof(bytes));
}
#endif

std::string realmFromBase(const std::string &base_dn) {
  std::string realm;
  std::string rest = base_dn;
  while (!rest.empty()) {
    const auto comma = rest.find(',');
    const std::string rdn = rest.substr(0, comma);
    std::string type;
    std::string value;
    if (parseRdn(rdn, type, value) && iequals(type, "dc")) {
      if (!realm.empty()) {
        realm.push_back('.');
      }
      realm += value;
    }
    if (comma == std::string::npos) {
      break;
    }
    rest = rest.substr(comma + 1);
  }
  return realm.empty() ? "simple-ldapd" : realm;
}

bool parsePlain(const std::string &credentials, std::string &authzid, std::string &authcid,
                std::string &password) {
  const auto first = credentials.find('\0');
  if (first == std::string::npos) {
    return false;
  }
  const auto second = credentials.find('\0', first + 1);
  if (second == std::string::npos) {
    return false;
  }
  authzid = credentials.substr(0, first);
  authcid = credentials.substr(first + 1, second - first - 1);
  password = credentials.substr(second + 1);
  return !authcid.empty();
}

std::string unquote(std::string value) {
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }
  return value;
}

std::map<std::string, std::string> parseDigestPairs(const std::string &text) {
  std::map<std::string, std::string> pairs;
  size_t pos = 0;
  while (pos < text.size()) {
    while (pos < text.size() && (text[pos] == ',' || std::isspace(static_cast<unsigned char>(text[pos])))) {
      ++pos;
    }
    if (pos >= text.size()) {
      break;
    }
    const size_t eq = text.find('=', pos);
    if (eq == std::string::npos) {
      break;
    }
    const std::string key = text.substr(pos, eq - pos);
    pos = eq + 1;
    std::string value;
    if (pos < text.size() && text[pos] == '"') {
      ++pos;
      while (pos < text.size() && text[pos] != '"') {
        if (text[pos] == '\\' && pos + 1 < text.size()) {
          ++pos;
        }
        value.push_back(text[pos]);
        ++pos;
      }
      if (pos < text.size() && text[pos] == '"') {
        ++pos;
      }
    } else {
      const size_t comma = text.find(',', pos);
      value = text.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
      pos = comma == std::string::npos ? text.size() : comma;
    }
    pairs[toLowerAscii(key)] = value;
  }
  return pairs;
}

SaslBindResult fail(ResultCode code, const std::string &diagnostic) {
  SaslBindResult result;
  result.result = code;
  result.diagnostic = diagnostic;
  return result;
}

}  // namespace

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

bool parseSaslMechanism(const std::string &name, SaslMechanism &mechanism) {
  if (iequals(name, "PLAIN")) {
    mechanism = SaslMechanism::Plain;
    return true;
  }
  if (iequals(name, "DIGEST-MD5")) {
    mechanism = SaslMechanism::DigestMd5;
    return true;
  }
  if (iequals(name, "EXTERNAL")) {
    mechanism = SaslMechanism::External;
    return true;
  }
  if (iequals(name, "GSSAPI")) {
    mechanism = SaslMechanism::Gssapi;
    return true;
  }
  if (iequals(name, "ANONYMOUS")) {
    mechanism = SaslMechanism::Anonymous;
    return true;
  }
  return false;
}

std::string saslPlainCredentials(const std::string &authzid, const std::string &authcid,
                                 const std::string &password) {
  return authzid + '\0' + authcid + '\0' + password;
}

bool saslDigestClientResponse(const std::string &challenge, const std::string &username,
                              const std::string &password, const std::string &digest_uri,
                              std::string &credentials, std::string &error) {
#ifdef SIMPLE_LDAPD_SSL
  const auto pairs = parseDigestPairs(challenge);
  const std::string realm = unquote(pairs.count("realm") ? pairs.at("realm") : "");
  const std::string nonce = unquote(pairs.count("nonce") ? pairs.at("nonce") : "");
  const std::string qop = unquote(pairs.count("qop") ? pairs.at("qop") : "auth");
  if (nonce.empty() || username.empty()) {
    error = "invalid DIGEST-MD5 challenge";
    return false;
  }
  const std::string cnonce = randomNonce();
  const std::string nc = "00000001";
  const std::string uri = digest_uri.empty() ? "ldap/" : digest_uri;
  const std::string ha1 =
      md5Hex(md5Raw(username + ":" + realm + ":" + password) + ":" + nonce + ":" + cnonce);
  const std::string ha2 = md5Hex("AUTHENTICATE:" + uri);
  const std::string response =
      md5Hex(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2);
  std::ostringstream out;
  out << "username=\"" << username << "\",realm=\"" << realm << "\",nonce=\"" << nonce
      << "\",cnonce=\"" << cnonce << "\",nc=" << nc << ",qop=" << qop
      << ",digest-uri=\"" << uri << "\",response=" << response << ",charset=utf-8";
  credentials = out.str();
  return true;
#else
  (void)challenge;
  (void)username;
  (void)password;
  (void)digest_uri;
  (void)credentials;
  error = "DIGEST-MD5 requires OpenSSL";
  return false;
#endif
}

void SaslAuthenticator::enable(SaslMechanism mechanism) {
  if (std::find(enabled_.begin(), enabled_.end(), mechanism) == enabled_.end()) {
    enabled_.push_back(mechanism);
  }
}

bool SaslAuthenticator::supports(SaslMechanism mechanism) const {
  return std::find(enabled_.begin(), enabled_.end(), mechanism) != enabled_.end();
}

std::vector<std::string> SaslAuthenticator::advertised() const {
  std::vector<std::string> names;
  names.reserve(enabled_.size());
  for (auto mechanism : enabled_) {
    names.emplace_back(toString(mechanism));
  }
  return names;
}

ResultCode SaslAuthenticator::step(SaslMechanism mechanism, const std::string & /*input*/,
                                   std::string &output) const {
  output.clear();
  return supports(mechanism) ? ResultCode::UnwillingToPerform
                             : ResultCode::AuthMethodNotSupported;
}

SaslBindResult SaslAuthenticator::bind(Backend &backend, const LdapConfig &config,
                                       const BindRequestData &request, bool tls,
                                       std::string &digest_nonce) const {
  SaslMechanism mechanism = SaslMechanism::Anonymous;
  if (!parseSaslMechanism(request.sasl_mechanism, mechanism) || !supports(mechanism)) {
    return fail(ResultCode::AuthMethodNotSupported, "unsupported SASL mechanism");
  }
  SimpleBindAuthenticator authenticator(backend, config);
  switch (mechanism) {
  case SaslMechanism::Plain: {
    if (config.require_confidentiality && !tls) {
      return fail(ResultCode::ConfidentialityRequired, "confidentiality required");
    }
    std::string authzid;
    std::string authcid;
    std::string password;
    if (!parsePlain(request.sasl_credentials, authzid, authcid, password)) {
      return fail(ResultCode::InvalidCredentials, "invalid PLAIN credentials");
    }
    const auto dn = authenticator.resolveName(authcid);
    if (!dn) {
      return fail(ResultCode::InvalidCredentials, "invalid credentials");
    }
    if (!authzid.empty() && !iequals(authzid, authcid) && !dnEquals(authzid, *dn)) {
      return fail(ResultCode::InsufficientAccessRights, "proxy authorization is not allowed");
    }
    if (authenticator.bind(*dn, password) != ResultCode::Success) {
      return fail(ResultCode::InvalidCredentials, "invalid credentials");
    }
    SaslBindResult result;
    result.result = ResultCode::Success;
    result.bind_dn = *dn;
    return result;
  }
  case SaslMechanism::DigestMd5: {
#ifdef SIMPLE_LDAPD_SSL
    if (digest_nonce.empty() || request.sasl_credentials.empty()) {
      digest_nonce = randomNonce();
      if (digest_nonce.empty()) {
        return fail(ResultCode::OperationsError, "DIGEST-MD5 nonce failed");
      }
      std::ostringstream challenge;
      challenge << "realm=\"" << realmFromBase(config.base_dn) << "\",nonce=\"" << digest_nonce
                << "\",qop=\"auth\",algorithm=md5-sess,charset=utf-8";
      SaslBindResult result;
      result.result = ResultCode::SaslBindInProgress;
      result.server_creds = challenge.str();
      return result;
    }
    const auto pairs = parseDigestPairs(request.sasl_credentials);
    const std::string username = unquote(pairs.count("username") ? pairs.at("username") : "");
    const std::string realm = unquote(pairs.count("realm") ? pairs.at("realm") : "");
    const std::string nonce = unquote(pairs.count("nonce") ? pairs.at("nonce") : "");
    const std::string cnonce = unquote(pairs.count("cnonce") ? pairs.at("cnonce") : "");
    const std::string nc = pairs.count("nc") ? pairs.at("nc") : "";
    const std::string qop = unquote(pairs.count("qop") ? pairs.at("qop") : "auth");
    const std::string uri = unquote(pairs.count("digest-uri") ? pairs.at("digest-uri") : "");
    const std::string response = unquote(pairs.count("response") ? pairs.at("response") : "");
    if (username.empty() || nonce != digest_nonce || cnonce.empty() || response.empty()) {
      digest_nonce.clear();
      return fail(ResultCode::InvalidCredentials, "invalid DIGEST-MD5 response");
    }
    const auto dn = authenticator.resolveName(username);
    const auto password = dn ? authenticator.passwordFor(*dn) : std::nullopt;
    if (!dn || !password) {
      digest_nonce.clear();
      return fail(ResultCode::InvalidCredentials, "invalid credentials");
    }
    const std::string ha1 =
        md5Hex(md5Raw(username + ":" + realm + ":" + *password) + ":" + nonce + ":" + cnonce);
    const std::string ha2 = md5Hex("AUTHENTICATE:" + uri);
    const std::string expected =
        md5Hex(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2);
    if (!iequals(expected, response)) {
      digest_nonce.clear();
      return fail(ResultCode::InvalidCredentials, "invalid credentials");
    }
    const std::string rspauth =
        md5Hex(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + md5Hex(":" + uri));
    digest_nonce.clear();
    SaslBindResult result;
    result.result = ResultCode::Success;
    result.bind_dn = *dn;
    result.server_creds = "rspauth=" + rspauth;
    return result;
#else
    (void)digest_nonce;
    return fail(ResultCode::AuthMethodNotSupported, "DIGEST-MD5 requires OpenSSL");
#endif
  }
  case SaslMechanism::External: {
    if (!tls) {
      return fail(ResultCode::ConfidentialityRequired, "EXTERNAL requires TLS");
    }
    const std::string identity =
        !request.sasl_credentials.empty() ? request.sasl_credentials : request.dn;
    const auto dn = authenticator.resolveName(identity);
    if (!dn) {
      return fail(ResultCode::InvalidCredentials, "EXTERNAL identity is unknown");
    }
    SaslBindResult result;
    result.result = ResultCode::Success;
    result.bind_dn = *dn;
    return result;
  }
  case SaslMechanism::Gssapi:
    return fail(ResultCode::AuthMethodNotSupported,
                "GSSAPI requires a ticket source (see v0.7.0)");
  case SaslMechanism::Anonymous:
    break;
  }
  return fail(ResultCode::AuthMethodNotSupported, "unsupported SASL mechanism");
}

}  // namespace simple_ldapd
