/**
 * @file config.cpp
 * @brief Key=value configuration parser
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/config/config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace simple_ldapd {

namespace {

std::string trim(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
              value.end());
  return value;
}

bool parseBool(const std::string &value) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

}  // namespace

LdapConfig::LdapConfig() = default;

bool LdapConfig::loadFromFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = trim(line.substr(0, eq));
    const std::string value = trim(line.substr(eq + 1));
    if (key == "listen_address") {
      listen_address = value;
    } else if (key == "ldap_port") {
      ldap_port = static_cast<port_t>(std::stoi(value));
    } else if (key == "ldaps_port") {
      ldaps_port = static_cast<port_t>(std::stoi(value));
    } else if (key == "enable_ldaps") {
      enable_ldaps = parseBool(value);
    } else if (key == "enable_starttls") {
      enable_starttls = parseBool(value);
    } else if (key == "tls_cert_file") {
      tls_cert_file = value;
    } else if (key == "tls_key_file") {
      tls_key_file = value;
    } else if (key == "tls_ca_file") {
      tls_ca_file = value;
    } else if (key == "backend") {
      backend = value;
    } else if (key == "ldif_file") {
      ldif_file = value;
    } else if (key == "schema_dir") {
      schema_dir = value;
    } else if (key == "base_dn") {
      base_dn = value;
    } else if (key == "root_dn") {
      root_dn = value;
    } else if (key == "root_password") {
      root_password = value;
    } else if (key == "log_file") {
      log_file = value;
    } else if (key == "log_level") {
      log_level = value;
    } else if (key == "foreground") {
      foreground = parseBool(value);
    } else if (key == "require_confidentiality") {
      require_confidentiality = parseBool(value);
    }
  }
  return true;
}

bool LdapConfig::validate() const {
  std::vector<std::string> errors;
  return validateDetailed(errors);
}

bool LdapConfig::validateDetailed(std::vector<std::string> &errors) const {
  errors.clear();
  if ((enable_ldaps || enable_starttls) &&
      (tls_cert_file.empty() || tls_key_file.empty())) {
    errors.emplace_back(
        "tls_cert_file and tls_key_file are required when TLS is enabled");
  }
  if (backend != "memory" && backend != "ldif") {
    errors.emplace_back("backend must be memory or ldif");
  }
  if (base_dn.empty()) {
    errors.emplace_back("base_dn is required");
  }
  return errors.empty();
}

}  // namespace simple_ldapd
