/**
 * @file test_ldap_config.cpp
 * @brief Configuration unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/protocol/filter.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include "simple-ldapd/utils/logger.hpp"
#include "simple-ldapd/version.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

std::string writeTempConfig() {
  const std::string path = "test-simple-ldapd.conf";
  std::ofstream out(path);
  out << "listen_address = 127.0.0.1\n";
  out << "ldap_port = 3389\n";
  out << "backend = memory\n";
  out << "base_dn = dc=example,dc=com\n";
  return path;
}

bool testDefaultConfig() {
  LdapConfig config;
  assert(config.ldap_port == kLdapDefaultPort);
  assert(config.ldaps_port == kLdapsDefaultPort);
  assert(config.backend == "memory");
  assert(config.validate());
  return true;
}

bool testInvalidBaseDn() {
  LdapConfig config;
  config.base_dn.clear();
  return !config.validate();
}

bool testEphemeralPortAllowed() {
  LdapConfig config;
  config.ldap_port = 0;
  return config.validate();
}

bool testEphemeralLdapsPortAllowed() {
  LdapConfig config;
  config.enable_ldaps = true;
  config.ldaps_port = 0;
  config.tls_cert_file = "server.crt";
  config.tls_key_file = "server.key";
  return config.validate();
}

bool testTlsRequiresCertificate() {
  LdapConfig config;
  config.enable_starttls = true;
  return !config.validate();
}

bool testRequireConfidentialityParse() {
  const std::string path = "test-simple-ldapd-tls.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "require_confidentiality = true\n";
  out << "enable_ldaps = true\n";
  out << "tls_cert_file = server.crt\n";
  out << "tls_key_file = server.key\n";
  out.close();
  LdapConfig config;
  return config.loadFromFile(path) && config.require_confidentiality && config.enable_ldaps &&
         config.validate();
}

bool testGssapiKeytabParse() {
  const std::string path = "test-simple-ldapd-gssapi.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "krb_realm = EXAMPLE.COM\n";
  out << "gssapi_service = ldap/localhost\n";
  out << "gssapi_keytab = missing.keytab\n";
  out.close();
  LdapConfig config;
  return config.loadFromFile(path) && config.krb_realm == "EXAMPLE.COM" &&
         config.gssapi_service == "ldap/localhost" && config.gssapi_keytab == "missing.keytab" &&
         !config.validate();
}

bool testFileLoad() {
  const auto path = writeTempConfig();
  LdapConfig config;
  if (!config.loadFromFile(path)) {
    return false;
  }
  return config.ldap_port == 3389 && config.listen_address == "127.0.0.1";
}

bool testFilterParse() {
  auto ok = SearchFilter::parse("(objectClass=*)");
  auto bad = SearchFilter::parse("objectClass=*");
  return ok.valid() && !bad.valid();
}

bool testResultCodes() {
  return std::string(toString(ResultCode::Unavailable)) == "unavailable" &&
         kVersion[0] != '\0';
}

bool testSqliteRequiresFile() {
  LdapConfig config;
  config.backend = "sqlite";
  return !config.validate();
}

bool testSqliteConfigParse() {
  const std::string path = "test-simple-ldapd-sqlite.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "backend = sqlite\n";
  out << "sqlite_file = /var/lib/simple-ldapd/directory.sqlite\n";
  out << "ldif_file = /var/lib/simple-ldapd/directory.ldif\n";
  out.close();
  LdapConfig config;
  return config.loadFromFile(path) && config.backend == "sqlite" &&
         config.sqlite_file == "/var/lib/simple-ldapd/directory.sqlite" &&
         config.ldif_file == "/var/lib/simple-ldapd/directory.ldif";
}

bool testSqliteValidatesWithFile() {
  LdapConfig config;
  config.backend = "sqlite";
  config.sqlite_file = "directory.sqlite";
#ifdef SIMPLE_LDAPD_SQLITE
  return config.validate();
#else
  return !config.validate();
#endif
}

bool testLogLevelParseAndApply() {
  const std::string path = "test-simple-ldapd-log.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "log_level = warning\n";
  out << "bind_rate_limit = 30\n";
  out << "tls_verify_client = false\n";
  out.close();
  LdapConfig config;
  LogLevel level = LogLevel::Info;
  if (!config.loadFromFile(path) || config.log_level != "warning" ||
      config.bind_rate_limit != 30 || !parseLogLevel(config.log_level, level) ||
      level != LogLevel::Warning || !config.validate()) {
    return false;
  }
  Logger::instance().setLevel(level);
  const bool applied = Logger::instance().level() == LogLevel::Warning;
  Logger::instance().setLevel(LogLevel::Info);
  return applied;
}

bool testInvalidLogLevel() {
  LdapConfig config;
  config.log_level = "verbose";
  return !config.validate();
}

bool testTlsVerifyClientRequiresCa() {
  LdapConfig config;
  config.tls_verify_client = true;
  return !config.validate();
}

bool testZeroPduSizeRejected() {
  LdapConfig config;
  config.max_pdu_size = 0;
  return !config.validate();
}

bool testSessionLimitsParse() {
  const std::string path = "test-simple-ldapd-limits.conf";
  std::ofstream out(path);
  out << "base_dn = dc=example,dc=com\n";
  out << "max_pdu_size = 65536\n";
  out << "max_sessions = 32\n";
  out << "idle_timeout = 120\n";
  out.close();
  LdapConfig config;
  return config.loadFromFile(path) && config.max_pdu_size == 65536 &&
         config.max_sessions == 32 && config.idle_timeout == 120 && config.validate();
}

}  // namespace

int main() {
  int passed = 0;
  int total = 0;
  auto run = [&](const char *name, bool (*fn)()) {
    ++total;
    if (fn()) {
      ++passed;
      std::cout << "PASS " << name << std::endl;
    } else {
      std::cout << "FAIL " << name << std::endl;
    }
  };
  run("testDefaultConfig", testDefaultConfig);
  run("testInvalidBaseDn", testInvalidBaseDn);
  run("testEphemeralPortAllowed", testEphemeralPortAllowed);
  run("testEphemeralLdapsPortAllowed", testEphemeralLdapsPortAllowed);
  run("testTlsRequiresCertificate", testTlsRequiresCertificate);
  run("testRequireConfidentialityParse", testRequireConfidentialityParse);
  run("testGssapiKeytabParse", testGssapiKeytabParse);
  run("testFileLoad", testFileLoad);
  run("testFilterParse", testFilterParse);
  run("testResultCodes", testResultCodes);
  run("testSqliteRequiresFile", testSqliteRequiresFile);
  run("testSqliteConfigParse", testSqliteConfigParse);
  run("testSqliteValidatesWithFile", testSqliteValidatesWithFile);
  run("testLogLevelParseAndApply", testLogLevelParseAndApply);
  run("testInvalidLogLevel", testInvalidLogLevel);
  run("testTlsVerifyClientRequiresCa", testTlsVerifyClientRequiresCa);
  run("testZeroPduSizeRejected", testZeroPduSizeRejected);
  run("testSessionLimitsParse", testSessionLimitsParse);
  std::cout << "Config tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
