/**
 * @file gssapi.hpp
 * @brief Lab GSSAPI tickets consumed on SASL bind
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace simple_ldapd {

struct GssapiKeytab {
  std::string realm;
  std::string service;
  std::string key;
};

std::string kerberosRealmFromBase(const std::string &base_dn);
std::string normalizeKerberosPrincipal(const std::string &name, const std::string &realm);
bool loadGssapiKeytab(const std::string &path, GssapiKeytab &keytab, std::string &error);
std::string encodeGssapiTicket(const GssapiKeytab &keytab, const std::string &principal,
                               std::int64_t issued, std::int64_t expiry);
std::string mintGssapiTicket(const GssapiKeytab &keytab, const std::string &principal,
                             int lifetime_seconds = 300);
bool verifyGssapiTicket(const GssapiKeytab &keytab, const std::string &credentials,
                        std::string &principal, std::string &error);

}  // namespace simple_ldapd
