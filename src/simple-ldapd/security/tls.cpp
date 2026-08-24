/**
 * @file tls.cpp
 * @brief OpenSSL TLS context
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/security/tls.hpp"

#ifdef SIMPLE_LDAPD_SSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

namespace simple_ldapd {

struct TlsContext::Impl {
#ifdef SIMPLE_LDAPD_SSL
  SSL_CTX *ctx{nullptr};
  ~Impl() {
    if (ctx != nullptr) {
      SSL_CTX_free(ctx);
    }
  }
#endif
};

namespace {

void ensureSsl() {
#ifdef SIMPLE_LDAPD_SSL
  static const bool initialized = [] {
    OPENSSL_init_ssl(0, nullptr);
    return true;
  }();
  (void)initialized;
#endif
}

#ifdef SIMPLE_LDAPD_SSL
SSL_CTX *makeCtx(const SSL_METHOD *method) {
  ensureSsl();
  SSL_CTX *ctx = SSL_CTX_new(method);
  if (ctx == nullptr) {
    return nullptr;
  }
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  return ctx;
}
#endif

}  // namespace

TlsContext::TlsContext() : impl_(std::make_unique<Impl>()) {}

TlsContext::~TlsContext() = default;

TlsContext::TlsContext(TlsContext &&other) noexcept = default;

TlsContext &TlsContext::operator=(TlsContext &&other) noexcept = default;

bool TlsContext::loadCertificate(const std::string &cert_file, const std::string &key_file) {
#ifdef SIMPLE_LDAPD_SSL
  if (impl_->ctx != nullptr) {
    SSL_CTX_free(impl_->ctx);
    impl_->ctx = nullptr;
  }
  SSL_CTX *ctx = makeCtx(TLS_server_method());
  if (ctx == nullptr) {
    return false;
  }
  if (SSL_CTX_use_certificate_file(ctx, cert_file.c_str(), SSL_FILETYPE_PEM) != 1 ||
      SSL_CTX_use_PrivateKey_file(ctx, key_file.c_str(), SSL_FILETYPE_PEM) != 1 ||
      SSL_CTX_check_private_key(ctx) != 1) {
    SSL_CTX_free(ctx);
    return false;
  }
  impl_->ctx = ctx;
  return true;
#else
  (void)cert_file;
  (void)key_file;
  return false;
#endif
}

bool TlsContext::loadCa(const std::string &ca_file) {
#ifdef SIMPLE_LDAPD_SSL
  if (impl_->ctx == nullptr) {
    return false;
  }
  return SSL_CTX_load_verify_locations(impl_->ctx, ca_file.c_str(), nullptr) == 1;
#else
  (void)ca_file;
  return false;
#endif
}

bool TlsContext::initClient(const std::string &ca_file) {
#ifdef SIMPLE_LDAPD_SSL
  if (impl_->ctx != nullptr) {
    SSL_CTX_free(impl_->ctx);
    impl_->ctx = nullptr;
  }
  SSL_CTX *ctx = makeCtx(TLS_client_method());
  if (ctx == nullptr) {
    return false;
  }
  if (!ca_file.empty()) {
    if (SSL_CTX_load_verify_locations(ctx, ca_file.c_str(), nullptr) != 1) {
      SSL_CTX_free(ctx);
      return false;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
  } else {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
  }
  impl_->ctx = ctx;
  return true;
#else
  (void)ca_file;
  return false;
#endif
}

bool TlsContext::enabled() const {
#ifdef SIMPLE_LDAPD_SSL
  return impl_ && impl_->ctx != nullptr;
#else
  return false;
#endif
}

void *TlsContext::native() const {
#ifdef SIMPLE_LDAPD_SSL
  return impl_ ? static_cast<void *>(impl_->ctx) : nullptr;
#else
  return nullptr;
#endif
}

}  // namespace simple_ldapd
