/**
 * @file tls.hpp
 * @brief TLS / StartTLS / LDAPS stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <string>

namespace simple_ldapd {

class TlsContext {
public:
  bool loadCertificate(const std::string &cert_file, const std::string &key_file);
  bool loadCa(const std::string &ca_file);
  bool enabled() const { return enabled_; }
  bool startTlsSupported() const { return enabled_; }

private:
  bool enabled_{false};
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
};

}  // namespace simple_ldapd
