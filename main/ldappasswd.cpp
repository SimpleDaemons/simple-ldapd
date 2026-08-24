/**
 * @file ldappasswd.cpp
 * @brief Change a user password
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/cli/common.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  using simple_ldapd::PasswordModifyRequest;
  using simple_ldapd::ResultCode;
  using simple_ldapd::cli::LdapClient;
  using simple_ldapd::cli::parseClientArgs;
  using simple_ldapd::cli::printPasswdUsage;
  using simple_ldapd::cli::printVersion;
  using simple_ldapd::toString;

  const auto options = parseClientArgs(argc, argv, true);
  if (options.parse_error) {
    printPasswdUsage();
    return 1;
  }
  if (options.help) {
    printPasswdUsage();
    return 0;
  }
  if (options.version) {
    printVersion("ldappasswd");
    return 0;
  }

  std::string new_password = options.new_password;
  if (options.prompt_new_password) {
    std::cerr << "New password: ";
    std::getline(std::cin, new_password);
    std::string confirm;
    std::cerr << "Re-enter new password: ";
    std::getline(std::cin, confirm);
    if (new_password != confirm) {
      std::cerr << "ldappasswd: passwords do not match" << std::endl;
      return 1;
    }
  }
  if (new_password.empty()) {
    std::cerr << "ldappasswd: specify -s PASSWORD or -S" << std::endl;
    return 1;
  }

  std::string error;
  auto client = LdapClient::openBound(options, error);
  if (!client) {
    std::cerr << "ldappasswd: " << error << std::endl;
    return 1;
  }

  PasswordModifyRequest request;
  if (!options.positionals.empty()) {
    request.user_identity = options.positionals.front();
  }
  if (!options.old_password.empty()) {
    request.old_password = options.old_password;
  }
  request.new_password = new_password;
  const ResultCode result = client->passwordModify(request);
  client->unbind();
  if (result != ResultCode::Success) {
    std::cerr << "ldappasswd: " << toString(result) << std::endl;
    return static_cast<int>(result);
  }
  std::cout << "Password Changed" << std::endl;
  return 0;
}
