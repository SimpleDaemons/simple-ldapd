/**
 * @file gssapi.cpp
 * @brief HMAC lab tickets for GSSAPI bind (no in-tree KDC)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/gssapi.hpp"

#include "simple-ldapd/utils/dn.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <exception>
#include <fstream>
#include <map>
#include <sstream>

#ifdef SIMPLE_LDAPD_SSL
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

namespace simple_ldapd {

namespace {

constexpr int kClockSkewSeconds = 60;

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

std::string toUpperAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return value;
}

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

bool constantTimeEqual(const std::string &left, const std::string &right) {
  const size_t size = left.size() > right.size() ? left.size() : right.size();
  unsigned char acc = static_cast<unsigned char>(left.size() ^ right.size());
  for (size_t i = 0; i < size; ++i) {
    const unsigned char a = i < left.size() ? static_cast<unsigned char>(left[i]) : 0;
    const unsigned char b = i < right.size() ? static_cast<unsigned char>(right[i]) : 0;
    acc = static_cast<unsigned char>(acc | (a ^ b));
  }
  return acc == 0;
}

std::string unquote(std::string value) {
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }
  return value;
}

std::map<std::string, std::string> parsePairs(const std::string &text) {
  std::map<std::string, std::string> pairs;
  size_t pos = 0;
  while (pos < text.size()) {
    while (pos < text.size() &&
           (text[pos] == ',' || std::isspace(static_cast<unsigned char>(text[pos])))) {
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
    pairs[toLowerAscii(trim(key))] = value;
  }
  return pairs;
}

std::string canonicalTicket(const std::string &realm, const std::string &principal,
                            const std::string &service, std::int64_t issued,
                            std::int64_t expiry) {
  std::ostringstream out;
  out << "v=1|realm|" << realm << "|principal|" << principal << "|service|" << service
      << "|issued|" << issued << "|expiry|" << expiry;
  return out.str();
}

#ifdef SIMPLE_LDAPD_SSL
std::string hmacSha256Hex(const std::string &key, const std::string &data) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int length = 0;
  HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char *>(data.data()), data.size(), digest, &length);
  return toHex(digest, length);
}
#endif

std::int64_t nowUnix() {
  return static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::system_clock::now().time_since_epoch())
                                       .count());
}

}  // namespace

std::string kerberosRealmFromBase(const std::string &base_dn) {
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
  return realm.empty() ? "SIMPLE-LDAPD" : toUpperAscii(realm);
}

std::string normalizeKerberosPrincipal(const std::string &name, const std::string &realm) {
  const auto at = name.rfind('@');
  if (at == std::string::npos) {
    return name;
  }
  const std::string local = name.substr(0, at);
  const std::string ticket_realm = name.substr(at + 1);
  if (!realm.empty() && !iequals(ticket_realm, realm)) {
    return {};
  }
  return local;
}

bool loadGssapiKeytab(const std::string &path, GssapiKeytab &keytab, std::string &error) {
  keytab = {};
  std::ifstream in(path);
  if (!in) {
    error = "cannot read GSSAPI keytab";
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = toLowerAscii(trim(line.substr(0, eq)));
    const std::string value = trim(line.substr(eq + 1));
    if (key == "realm") {
      keytab.realm = toUpperAscii(value);
    } else if (key == "service") {
      keytab.service = value;
    } else if (key == "key") {
      keytab.key = value;
    }
  }
  if (keytab.key.empty()) {
    error = "GSSAPI keytab is missing key";
    return false;
  }
  if (keytab.service.empty()) {
    keytab.service = "ldap/localhost";
  }
  return true;
}

std::string encodeGssapiTicket(const GssapiKeytab &keytab, const std::string &principal,
                               std::int64_t issued, std::int64_t expiry) {
#ifdef SIMPLE_LDAPD_SSL
  const std::string local = normalizeKerberosPrincipal(principal, keytab.realm);
  if (local.empty() || keytab.key.empty() || keytab.realm.empty() || keytab.service.empty()) {
    return {};
  }
  const std::string mac =
      hmacSha256Hex(keytab.key, canonicalTicket(keytab.realm, local, keytab.service, issued, expiry));
  std::ostringstream out;
  out << "v=1,realm=\"" << keytab.realm << "\",principal=\"" << local << "\",service=\""
      << keytab.service << "\",issued=" << issued << ",expiry=" << expiry << ",mac=" << mac;
  return out.str();
#else
  (void)keytab;
  (void)principal;
  (void)issued;
  (void)expiry;
  return {};
#endif
}

std::string mintGssapiTicket(const GssapiKeytab &keytab, const std::string &principal,
                             int lifetime_seconds) {
  if (lifetime_seconds <= 0) {
    return {};
  }
  const auto issued = nowUnix();
  return encodeGssapiTicket(keytab, principal, issued, issued + lifetime_seconds);
}

bool verifyGssapiTicket(const GssapiKeytab &keytab, const std::string &credentials,
                        std::string &principal, std::string &error) {
  principal.clear();
#ifdef SIMPLE_LDAPD_SSL
  if (keytab.key.empty() || keytab.realm.empty() || keytab.service.empty()) {
    error = "GSSAPI keytab is incomplete";
    return false;
  }
  const auto pairs = parsePairs(credentials);
  const std::string version = unquote(pairs.count("v") ? pairs.at("v") : "");
  const std::string realm = unquote(pairs.count("realm") ? pairs.at("realm") : "");
  const std::string name = unquote(pairs.count("principal") ? pairs.at("principal") : "");
  const std::string service = unquote(pairs.count("service") ? pairs.at("service") : "");
  const std::string issued_text = unquote(pairs.count("issued") ? pairs.at("issued") : "");
  const std::string expiry_text = unquote(pairs.count("expiry") ? pairs.at("expiry") : "");
  const std::string mac = unquote(pairs.count("mac") ? pairs.at("mac") : "");
  if (version != "1" || name.empty() || issued_text.empty() || expiry_text.empty() || mac.empty()) {
    error = "invalid GSSAPI ticket";
    return false;
  }
  if (!iequals(realm, keytab.realm) || !iequals(service, keytab.service)) {
    error = "GSSAPI ticket realm or service mismatch";
    return false;
  }
  std::int64_t issued = 0;
  std::int64_t expiry = 0;
  try {
    issued = static_cast<std::int64_t>(std::stoll(issued_text));
    expiry = static_cast<std::int64_t>(std::stoll(expiry_text));
  } catch (const std::exception &) {
    error = "invalid GSSAPI ticket timestamps";
    return false;
  }
  const auto now = nowUnix();
  if (expiry < now - kClockSkewSeconds || issued > now + kClockSkewSeconds || issued >= expiry) {
    error = "GSSAPI ticket expired";
    return false;
  }
  const std::string expected =
      hmacSha256Hex(keytab.key, canonicalTicket(keytab.realm, name, keytab.service, issued, expiry));
  if (!constantTimeEqual(toLowerAscii(mac), expected)) {
    error = "invalid GSSAPI ticket";
    return false;
  }
  principal = name;
  return true;
#else
  (void)keytab;
  (void)credentials;
  error = "GSSAPI requires OpenSSL";
  return false;
#endif
}

}  // namespace simple_ldapd
