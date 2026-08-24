/**
 * @file tls.hpp
 * @brief TLS context for LDAPS and StartTLS
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <memory>
#include <string>

namespace simple_ldapd {

inline constexpr const char *kStartTlsOid = "1.3.6.1.4.1.1466.20037";

class TlsContext {
public:
  TlsContext();
  ~TlsContext();
  TlsContext(TlsContext &&other) noexcept;
  TlsContext &operator=(TlsContext &&other) noexcept;
  TlsContext(const TlsContext &) = delete;
  TlsContext &operator=(const TlsContext &) = delete;

  bool loadCertificate(const std::string &cert_file, const std::string &key_file);
  bool loadCa(const std::string &ca_file);
  bool initClient(const std::string &ca_file = {});
  bool enabled() const;
  bool startTlsSupported() const { return enabled(); }
  void *native() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace simple_ldapd
