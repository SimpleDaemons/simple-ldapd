/**
 * @file tls.cpp
 * @brief TLS context stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/tls.hpp"

#include <fstream>

namespace simple_ldapd {

bool TlsContext::loadCertificate(const std::string &cert_file,
                                 const std::string &key_file) {
  std::ifstream cert(cert_file);
  std::ifstream key(key_file);
  enabled_ = static_cast<bool>(cert) && static_cast<bool>(key);
  cert_file_ = cert_file;
  key_file_ = key_file;
  return enabled_;
}

bool TlsContext::loadCa(const std::string &ca_file) {
  std::ifstream ca(ca_file);
  if (!ca) {
    return false;
  }
  ca_file_ = ca_file;
  return true;
}

}  // namespace simple_ldapd
