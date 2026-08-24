/**
 * @file session.hpp
 * @brief LDAP session state
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/utils/net.hpp"
#include <atomic>
#include <string>

namespace simple_ldapd {

class TlsContext;
class SchemaRegistry;
class SaslAuthenticator;

class Session {
public:
  Session(TcpConnection connection, Backend &backend, const LdapConfig &config,
          std::atomic<bool> &running, TlsContext *tls = nullptr,
          SchemaRegistry *schema = nullptr, SaslAuthenticator *sasl = nullptr);

  bool bound() const { return bound_; }
  const std::string &bindDn() const { return bind_dn_; }
  void serve();

private:
  bool send(const LdapMessage &message);
  bool mayWrite() const;
  LdapMessage handleBind(const LdapMessage &request);
  bool handleSearch(const LdapMessage &request);
  LdapMessage handleAdd(const LdapMessage &request);
  LdapMessage handleModify(const LdapMessage &request);
  LdapMessage handleDelete(const LdapMessage &request);
  LdapMessage handleModifyDn(const LdapMessage &request);
  SearchEntryData toSearchEntry(const DirectoryEntry &entry,
                                const SearchRequestData &request) const;
  ResultCode checkSchema(const DirectoryEntry &entry, std::string &diagnostic) const;
  DirectoryEntry rootDse() const;

  TcpConnection connection_;
  Backend &backend_;
  const LdapConfig &config_;
  std::atomic<bool> &running_;
  TlsContext *tls_{nullptr};
  SchemaRegistry *schema_{nullptr};
  SaslAuthenticator *sasl_{nullptr};
  bool bound_{false};
  std::string bind_dn_;
  std::string sasl_digest_nonce_;
};

}  // namespace simple_ldapd
