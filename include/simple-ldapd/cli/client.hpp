/**
 * @file client.hpp
 * @brief Minimal LDAPv3 client helper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/cli/common.hpp"
#include "simple-ldapd/protocol/message.hpp"
#include "simple-ldapd/utils/net.hpp"
#include <optional>
#include <string>

namespace simple_ldapd {
namespace cli {

std::optional<TcpConnection> connectLdap(const ClientOptions &options, std::string &error);

class LdapClient {
public:
  static std::optional<LdapClient> connect(const std::string &host, port_t port);
  static std::optional<LdapClient> openBound(const ClientOptions &options, std::string &error);

  ResultCode simpleBind(const std::string &dn, const std::string &password);
  ResultCode add(const DirectoryEntry &entry);
  ResultCode modify(const std::string &dn, const std::vector<AttributeModification> &changes);
  ResultCode del(const std::string &dn);
  ResultCode modifyDn(const ModifyDnRequestData &request);
  void unbind();

private:
  explicit LdapClient(TcpConnection connection);
  std::optional<LdapMessage> exchange(const LdapMessage &request);

  TcpConnection connection_;
  int next_id_{1};
};

}  // namespace cli
}  // namespace simple_ldapd
