/**
 * @file ldif.hpp
 * @brief LDIF import/export backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/memory.hpp"
#include <string>

namespace simple_ldapd {

class LdifBackend : public MemoryBackend {
public:
  explicit LdifBackend(std::string path);

  bool initialize() override;
  std::string name() const override { return "ldif"; }

  bool importFile(const std::string &path);
  bool exportFile(const std::string &path) const;

private:
  std::string path_;
};

}  // namespace simple_ldapd
