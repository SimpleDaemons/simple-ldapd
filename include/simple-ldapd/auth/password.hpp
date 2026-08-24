/**
 * @file password.hpp
 * @brief userPassword hashing and account-control checks
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include <optional>
#include <string>
#include <vector>

namespace simple_ldapd {

bool verifyUserPassword(const std::string &stored, const std::string &provided);
std::string encodeUserPassword(const std::string &value);
std::optional<std::string> recoverablePassword(const std::string &stored);
bool accountDisabled(const DirectoryEntry &entry);
void encodeUserPasswords(DirectoryEntry &entry);
void encodeUserPasswordChanges(std::vector<AttributeModification> &changes);

}  // namespace simple_ldapd
