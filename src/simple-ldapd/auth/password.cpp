/**
 * @file password.cpp
 * @brief userPassword hashing and account-control checks
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/auth/password.hpp"

#include "simple-ldapd/utils/dn.hpp"
#include <cstdlib>
#include <vector>

#ifdef SIMPLE_LDAPD_SSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#endif

namespace simple_ldapd {

namespace {

constexpr unsigned kSha1Length = 20;
constexpr unsigned kSshaSaltLength = 8;
constexpr unsigned kAccountDisableBit = 0x0002;

bool constantTimeEqual(const unsigned char *left, const unsigned char *right, size_t size) {
  unsigned char acc = 0;
  for (size_t i = 0; i < size; ++i) {
    acc = static_cast<unsigned char>(acc | (left[i] ^ right[i]));
  }
  return acc == 0;
}

bool passwordsEqual(const std::string &left, const std::string &right) {
  const size_t size = left.size() > right.size() ? left.size() : right.size();
  unsigned char acc = static_cast<unsigned char>(left.size() ^ right.size());
  for (size_t i = 0; i < size; ++i) {
    const unsigned char a = i < left.size() ? static_cast<unsigned char>(left[i]) : 0;
    const unsigned char b = i < right.size() ? static_cast<unsigned char>(right[i]) : 0;
    acc = static_cast<unsigned char>(acc | (a ^ b));
  }
  return acc == 0;
}

bool schemePrefix(const std::string &value, std::string &scheme, std::string &rest) {
  if (value.size() < 3 || value.front() != '{') {
    return false;
  }
  const auto close = value.find('}');
  if (close == std::string::npos || close < 2) {
    return false;
  }
  scheme = value.substr(1, close - 1);
  rest = value.substr(close + 1);
  return true;
}

std::string base64Encode(const unsigned char *data, size_t length) {
  static const char kTable[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((length + 2) / 3) * 4);
  for (size_t i = 0; i < length; i += 3) {
    unsigned n = static_cast<unsigned>(data[i]) << 16;
    if (i + 1 < length) {
      n |= static_cast<unsigned>(data[i + 1]) << 8;
    }
    if (i + 2 < length) {
      n |= static_cast<unsigned>(data[i + 2]);
    }
    out.push_back(kTable[(n >> 18) & 63]);
    out.push_back(kTable[(n >> 12) & 63]);
    out.push_back(i + 1 < length ? kTable[(n >> 6) & 63] : '=');
    out.push_back(i + 2 < length ? kTable[n & 63] : '=');
  }
  return out;
}

int base64Value(char ch) {
  if (ch >= 'A' && ch <= 'Z') {
    return ch - 'A';
  }
  if (ch >= 'a' && ch <= 'z') {
    return ch - 'a' + 26;
  }
  if (ch >= '0' && ch <= '9') {
    return ch - '0' + 52;
  }
  if (ch == '+') {
    return 62;
  }
  if (ch == '/') {
    return 63;
  }
  return -1;
}

std::vector<unsigned char> base64Decode(const std::string &text) {
  std::vector<unsigned char> out;
  int val = 0;
  int bits = -8;
  for (char ch : text) {
    if (ch == '=' || ch == '\n' || ch == '\r' || ch == ' ') {
      continue;
    }
    const int digit = base64Value(ch);
    if (digit < 0) {
      return {};
    }
    val = (val << 6) + digit;
    bits += 6;
    if (bits >= 0) {
      out.push_back(static_cast<unsigned char>((val >> bits) & 0xff));
      bits -= 8;
    }
  }
  return out;
}

bool sha1(const unsigned char *data, size_t length, unsigned char *digest) {
#ifdef SIMPLE_LDAPD_SSL
  unsigned int out_length = 0;
  return EVP_Digest(data, length, digest, &out_length, EVP_sha1(), nullptr) == 1 &&
         out_length == kSha1Length;
#else
  (void)data;
  (void)length;
  (void)digest;
  return false;
#endif
}

std::string hashSsha(const std::string &password) {
#ifdef SIMPLE_LDAPD_SSL
  unsigned char salt[kSshaSaltLength];
  if (RAND_bytes(salt, static_cast<int>(sizeof(salt))) != 1) {
    return {};
  }
  std::vector<unsigned char> material(password.begin(), password.end());
  material.insert(material.end(), salt, salt + sizeof(salt));
  unsigned char digest[kSha1Length];
  if (!sha1(material.data(), material.size(), digest)) {
    return {};
  }
  std::vector<unsigned char> payload(digest, digest + kSha1Length);
  payload.insert(payload.end(), salt, salt + sizeof(salt));
  return "{SSHA}" + base64Encode(payload.data(), payload.size());
#else
  (void)password;
  return {};
#endif
}

bool verifySha(const std::string &payload, const std::string &password) {
  const auto decoded = base64Decode(payload);
  if (decoded.size() != kSha1Length) {
    return false;
  }
  unsigned char digest[kSha1Length];
  if (!sha1(reinterpret_cast<const unsigned char *>(password.data()), password.size(),
            digest)) {
    return false;
  }
  return constantTimeEqual(decoded.data(), digest, kSha1Length);
}

bool verifySsha(const std::string &payload, const std::string &password) {
  const auto decoded = base64Decode(payload);
  if (decoded.size() <= kSha1Length) {
    return false;
  }
  std::vector<unsigned char> material(password.begin(), password.end());
  material.insert(material.end(), decoded.begin() + kSha1Length, decoded.end());
  unsigned char digest[kSha1Length];
  if (!sha1(material.data(), material.size(), digest)) {
    return false;
  }
  return constantTimeEqual(decoded.data(), digest, kSha1Length);
}

const std::vector<std::string> *findAttribute(const DirectoryEntry &entry,
                                              const std::string &name) {
  for (const auto &pair : entry.attributes) {
    if (iequals(pair.first, name)) {
      return &pair.second;
    }
  }
  return nullptr;
}

}  // namespace

bool verifyUserPassword(const std::string &stored, const std::string &provided) {
  std::string scheme;
  std::string rest;
  if (!schemePrefix(stored, scheme, rest)) {
    return passwordsEqual(provided, stored);
  }
  if (iequals(scheme, "CLEARTEXT")) {
    return passwordsEqual(provided, rest);
  }
  if (iequals(scheme, "SSHA")) {
    return verifySsha(rest, provided);
  }
  if (iequals(scheme, "SHA")) {
    return verifySha(rest, provided);
  }
  return false;
}

std::string encodeUserPassword(const std::string &value) {
  std::string scheme;
  std::string rest;
  if (schemePrefix(value, scheme, rest)) {
    return value;
  }
  if (value.empty()) {
    return value;
  }
  const std::string hashed = hashSsha(value);
  return hashed.empty() ? value : hashed;
}

std::optional<std::string> recoverablePassword(const std::string &stored) {
  std::string scheme;
  std::string rest;
  if (!schemePrefix(stored, scheme, rest)) {
    return stored;
  }
  if (iequals(scheme, "CLEARTEXT")) {
    return rest;
  }
  return std::nullopt;
}

bool accountDisabled(const DirectoryEntry &entry) {
  const auto *values = findAttribute(entry, "userAccountControl");
  if (values == nullptr) {
    return false;
  }
  for (const auto &value : *values) {
    char *end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || (end != nullptr && *end != '\0') || parsed < 0) {
      continue;
    }
    if ((static_cast<unsigned long>(parsed) & kAccountDisableBit) != 0) {
      return true;
    }
  }
  return false;
}

void encodeUserPasswords(DirectoryEntry &entry) {
  for (auto &pair : entry.attributes) {
    if (!iequals(pair.first, "userPassword")) {
      continue;
    }
    for (auto &value : pair.second) {
      value = encodeUserPassword(value);
    }
  }
}

void encodeUserPasswordChanges(std::vector<AttributeModification> &changes) {
  for (auto &change : changes) {
    if (change.op == ModifyOp::Delete || !iequals(change.type, "userPassword")) {
      continue;
    }
    for (auto &value : change.values) {
      value = encodeUserPassword(value);
    }
  }
}

}  // namespace simple_ldapd
