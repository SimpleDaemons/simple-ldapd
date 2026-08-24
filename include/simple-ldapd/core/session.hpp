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

class Session {
public:
  Session(TcpConnection connection, Backend &backend, const LdapConfig &config,
          std::atomic<bool> &running);

  bool bound() const { return bound_; }
  const std::string &bindDn() const { return bind_dn_; }
  void serve();

private:
  bool send(const LdapMessage &message);
  LdapMessage handleBind(const LdapMessage &request);
  bool handleSearch(const LdapMessage &request);
  SearchEntryData toSearchEntry(const DirectoryEntry &entry,
                                const SearchRequestData &request) const;

  TcpConnection connection_;
  Backend &backend_;
  const LdapConfig &config_;
  std::atomic<bool> &running_;
  bool bound_{false};
  std::string bind_dn_;
};

}  // namespace simple_ldapd
