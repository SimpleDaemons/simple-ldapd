/**
 * @file common.cpp
 * @brief Shared CLI argument parsing
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/common.hpp"

#include "simple-ldapd/version.hpp"
#include <iostream>
#include <stdexcept>

namespace simple_ldapd {
namespace cli {

namespace {

void applyUri(ClientOptions &options, const std::string &uri) {
  options.uri = uri;
  std::string rest = uri;
  options.ldaps = false;
  options.port = kLdapDefaultPort;
  if (rest.rfind("ldaps://", 0) == 0) {
    options.ldaps = true;
    options.port = kLdapsDefaultPort;
    rest = rest.substr(8);
  } else if (rest.rfind("ldap://", 0) == 0) {
    rest = rest.substr(7);
  }
  const auto slash = rest.find('/');
  if (slash != std::string::npos) {
    if (options.base_dn.empty() && slash + 1 < rest.size()) {
      options.base_dn = rest.substr(slash + 1);
    }
    rest = rest.substr(0, slash);
  }
  const auto colon = rest.rfind(':');
  if (colon != std::string::npos && rest.find(']') == std::string::npos) {
    options.host = rest.substr(0, colon);
    try {
      const int parsed = std::stoi(rest.substr(colon + 1));
      if (parsed < 0 || parsed > 65535) {
        options.parse_error = true;
        return;
      }
      options.port = static_cast<port_t>(parsed);
    } catch (const std::exception &) {
      options.parse_error = true;
      return;
    }
  } else {
    options.host = rest;
  }
  if (options.host.empty()) {
    options.host = "127.0.0.1";
  }
}

}  // namespace

void printClientUsage(const std::string &tool, const std::string &summary) {
  std::cout << "Usage: " << tool << " [options] [filter [attributes...]]\n"
            << summary << "\n\n"
            << "Options:\n"
            << "  -H URI         LDAP URI (ldap://host:port or ldaps://host:port)\n"
            << "  -h HOST        LDAP host (OpenLDAP-compatible)\n"
            << "  -p PORT        LDAP port\n"
            << "  -Z             StartTLS after connect\n"
            << "  --ca-file FILE Trust CA (or server cert) for TLS\n"
            << "  --cert FILE    Client certificate for SASL EXTERNAL\n"
            << "  --key FILE     Client private key for SASL EXTERNAL\n"
            << "  -x             Simple authentication\n"
            << "  -Y MECH        SASL mechanism (PLAIN, DIGEST-MD5, EXTERNAL, GSSAPI)\n"
            << "  -U AUTHCID     SASL authentication identity\n"
            << "  --keytab FILE  Lab GSSAPI keytab (or SIMPLE_LDAPD_KTNAME)\n"
            << "  -D BIND_DN     Bind DN\n"
            << "  -w PASSWORD    Bind password\n"
            << "  -W             Prompt for bind password\n"
            << "  -b BASE_DN     Search base DN\n"
            << "  -s SCOPE       base, one, or sub (default: sub)\n"
            << "  -A             Types only (attribute names, no values)\n"
            << "  -l SECONDS     Search time limit (0 = unlimited)\n"
            << "  -z COUNT       Search size limit (0 = unlimited)\n"
            << "  -E pr=N        Paged results (RFC 2696), N entries per page\n"
            << "  -a             Treat LDIF records as add (ldapmodify)\n"
            << "  -f FILE        LDIF file\n"
            << "  --help         Show this help\n"
            << "  --version      Show version\n";
}

void printPasswdUsage() {
  std::cout << "Usage: ldappasswd [options] [user]\n"
            << "Change a user password\n\n"
            << "Options:\n"
            << "  -H URI         LDAP URI (ldap://host:port or ldaps://host:port)\n"
            << "  -h HOST        LDAP host (OpenLDAP-compatible)\n"
            << "  -p PORT        LDAP port\n"
            << "  -Z             StartTLS after connect\n"
            << "  --ca-file FILE Trust CA (or server cert) for TLS\n"
            << "  -x             Simple authentication\n"
            << "  -Y MECH        SASL mechanism (PLAIN, DIGEST-MD5, EXTERNAL, GSSAPI)\n"
            << "  -U AUTHCID     SASL authentication identity\n"
            << "  --keytab FILE  Lab GSSAPI keytab (or SIMPLE_LDAPD_KTNAME)\n"
            << "  -D BIND_DN     Bind DN\n"
            << "  -w PASSWORD    Bind password\n"
            << "  -W             Prompt for bind password\n"
            << "  -s PASSWORD    New password\n"
            << "  -a PASSWORD    Old password (optional if already bound as the user)\n"
            << "  -S             Prompt for the new password\n"
            << "  --help         Show this help\n"
            << "  --version      Show version\n";
}

void printVersion(const std::string &tool) {
  std::cout << tool << " (simple-ldapd) " << kVersion << std::endl;
}

ClientOptions parseClientArgs(int argc, char *argv[], bool passwd) {
  ClientOptions options;
  bool port_override = false;
  port_t explicit_port = kLdapDefaultPort;
  bool positional_filter = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) {
        options.parse_error = true;
        return {};
      }
      return argv[++i];
    };
    if (arg == "--help") {
      options.help = true;
    } else if (arg == "--version") {
      options.version = true;
    } else if (arg == "-H") {
      applyUri(options, next());
    } else if (arg == "-h") {
      applyUri(options, "ldap://" + next());
    } else if (arg == "-p") {
      try {
        const int parsed = std::stoi(next());
        if (parsed < 0 || parsed > 65535) {
          options.parse_error = true;
        } else {
          port_override = true;
          explicit_port = static_cast<port_t>(parsed);
        }
      } catch (const std::exception &) {
        options.parse_error = true;
      }
    } else if (arg == "-x") {
      options.simple_auth = true;
    } else if (arg == "-Y") {
      options.sasl_mechanism = next();
      options.simple_auth = false;
    } else if (arg == "-U") {
      options.sasl_authcid = next();
    } else if (arg == "--keytab") {
      options.keytab = next();
    } else if (arg == "-Z") {
      options.starttls = true;
    } else if (arg == "--ca-file") {
      options.ca_file = next();
    } else if (arg == "--cert") {
      options.tls_cert_file = next();
    } else if (arg == "--key") {
      options.tls_key_file = next();
    } else if (arg == "-a") {
      if (passwd) {
        options.old_password = next();
      } else {
        options.add_mode = true;
      }
    } else if (arg == "-S" && passwd) {
      options.prompt_new_password = true;
    } else if (arg == "-D") {
      options.bind_dn = next();
    } else if (arg == "-w") {
      options.password = next();
    } else if (arg == "-W") {
      options.prompt_password = true;
    } else if (arg == "-b") {
      options.base_dn = next();
    } else if (arg == "-s") {
      const std::string value = next();
      if (passwd) {
        options.new_password = value;
      } else if (value == "base") {
        options.scope = SearchScope::Base;
      } else if (value == "one") {
        options.scope = SearchScope::OneLevel;
      } else if (value == "sub") {
        options.scope = SearchScope::Subtree;
      } else {
        options.parse_error = true;
      }
    } else if (arg == "-A") {
      options.types_only = true;
    } else if (arg == "-l") {
      try {
        options.time_limit = std::stoi(next());
      } catch (const std::exception &) {
        options.parse_error = true;
      }
    } else if (arg == "-z") {
      try {
        options.size_limit = std::stoi(next());
      } catch (const std::exception &) {
        options.parse_error = true;
      }
    } else if (arg == "-E") {
      const std::string value = next();
      auto eq = value.find('=');
      if (eq == std::string::npos || value.compare(0, eq, "pr") != 0) {
        options.parse_error = true;
      } else {
        std::string count = value.substr(eq + 1);
        const auto slash = count.find('/');
        if (slash != std::string::npos) {
          count = count.substr(0, slash);
        }
        try {
          options.page_size = std::stoi(count);
        } catch (const std::exception &) {
          options.parse_error = true;
        }
      }
    } else if (arg == "-f") {
      options.ldif_file = next();
    } else if (!arg.empty() && arg[0] != '-') {
      options.positionals.push_back(arg);
      if (!positional_filter) {
        options.filter = arg;
        positional_filter = true;
      } else {
        options.attributes.push_back(arg);
      }
    }
  }
  if (options.uri.rfind("ldap", 0) == 0) {
    applyUri(options, options.uri);
  }
  if (port_override) {
    options.port = explicit_port;
  }
  if (!passwd && !options.filter.empty() && options.filter.front() != '(') {
    options.filter = "(" + options.filter + ")";
  }
  return options;
}

int notImplementedExit(const std::string &tool) {
  std::cerr << tool << ": LDAP client operations are not implemented yet"
            << std::endl;
  return 2;
}

}  // namespace cli
}  // namespace simple_ldapd
