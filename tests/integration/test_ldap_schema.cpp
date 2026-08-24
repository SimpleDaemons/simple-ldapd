/**
 * @file test_ldap_schema.cpp
 * @brief Schema parser and validation tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/cli/client.hpp"
#include "simple-ldapd/config/config.hpp"
#include "simple-ldapd/core/daemon.hpp"
#include "simple-ldapd/schema/registry.hpp"

#include <fstream>
#include <iostream>
#include <string>

using namespace simple_ldapd;

namespace {

std::string schemaDir() {
  for (const char *path : {"schemas", "../schemas"}) {
    SchemaRegistry registry;
    if (registry.loadDirectory(path) && registry.hasObjectClass("inetOrgPerson") &&
        registry.hasAttribute("sAMAccountName")) {
      return path;
    }
  }
  return {};
}

bool testParseNamesAndSyntax() {
  const std::string path = "test-schema-multiline.schema";
  std::ofstream out(path);
  out << "attributetype ( 2.5.4.41 NAME 'name'\n"
      << "  SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 )\n"
      << "attributetype ( 2.5.4.3 NAME ( 'cn' 'commonName' )\n"
      << "  SUP name )\n"
      << "attributetype ( 2.5.4.4 NAME 'sn' SUP name )\n"
      << "objectclass ( 2.5.6.0 NAME 'top' ABSTRACT MUST objectClass )\n"
      << "objectclass ( 2.5.6.6 NAME 'person' SUP top STRUCTURAL\n"
      << "  MUST ( sn $ cn ) )\n";
  out.close();
  SchemaRegistry registry;
  if (!registry.loadFile(path)) {
    return false;
  }
  const SchemaAttribute *cn = registry.findAttribute("commonName");
  return registry.hasAttribute("cn") && cn != nullptr && cn->superior == "name" &&
         registry.hasObjectClass("PERSON");
}

bool testProjectSchemas() {
  const std::string dir = schemaDir();
  if (dir.empty()) {
    return false;
  }
  SchemaRegistry registry;
  return registry.loadDirectory(dir) && registry.hasObjectClass("posixAccount") &&
         registry.hasObjectClass("inetOrgPerson") && registry.hasObjectClass("user") &&
         registry.hasAttribute("memberOf");
}

DirectoryEntry validPerson() {
  DirectoryEntry entry;
  entry.dn = "uid=bob,dc=example,dc=com";
  entry.attributes["objectClass"].push_back("inetOrgPerson");
  entry.attributes["cn"].push_back("Bob");
  entry.attributes["sn"].push_back("Example");
  entry.attributes["uid"].push_back("bob");
  return entry;
}

bool testValidateInetOrgPerson() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  return registry.validateEntry(validPerson()) == ResultCode::Success;
}

bool testMissingMust() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  auto entry = validPerson();
  entry.attributes.erase("sn");
  std::string diagnostic;
  return registry.validateEntry(entry, diagnostic) == ResultCode::ObjectClassViolation &&
         diagnostic.find("sn") != std::string::npos;
}

bool testUndefinedAttribute() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  auto entry = validPerson();
  entry.attributes["notARealAttribute"].push_back("x");
  return registry.validateEntry(entry) == ResultCode::UndefinedAttributeType;
}

bool testNotAllowedAttribute() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  auto entry = validPerson();
  entry.attributes["uidNumber"].push_back("1001");
  return registry.validateEntry(entry) == ResultCode::ObjectClassViolation;
}

bool testPosixAndAdCompat() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  auto entry = validPerson();
  entry.attributes["objectClass"].push_back("posixAccount");
  entry.attributes["objectClass"].push_back("user");
  entry.attributes["uidNumber"].push_back("1001");
  entry.attributes["gidNumber"].push_back("1001");
  entry.attributes["homeDirectory"].push_back("/home/bob");
  entry.attributes["sAMAccountName"].push_back("bob");
  entry.attributes["memberOf"].push_back("cn=developers,ou=Groups,dc=example,dc=com");
  if (registry.validateEntry(entry) != ResultCode::Success) {
    return false;
  }
  entry.attributes["uidNumber"] = {"nope"};
  return registry.validateEntry(entry) == ResultCode::InvalidAttributeSyntax;
}

bool testTwoStructuralClasses() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  auto entry = validPerson();
  entry.attributes["objectClass"].push_back("organization");
  entry.attributes["o"].push_back("Example");
  return registry.validateEntry(entry) == ResultCode::ObjectClassViolation;
}

bool testSingleValue() {
  SchemaRegistry registry;
  if (!registry.loadDirectory(schemaDir())) {
    return false;
  }
  DirectoryEntry entry;
  entry.dn = "dc=example,dc=com";
  entry.attributes["objectClass"].push_back("organization");
  entry.attributes["objectClass"].push_back("dcObject");
  entry.attributes["o"].push_back("Example");
  entry.attributes["dc"].push_back("example");
  entry.attributes["dc"].push_back("extra");
  return registry.validateEntry(entry) == ResultCode::ConstraintViolation;
}

bool seedBase(LdapDaemon &daemon) {
  DirectoryEntry base;
  base.dn = "dc=example,dc=com";
  base.attributes["objectClass"].push_back("organization");
  base.attributes["objectClass"].push_back("dcObject");
  base.attributes["o"].push_back("Example");
  base.attributes["dc"].push_back("example");
  DirectoryEntry people;
  people.dn = "ou=People,dc=example,dc=com";
  people.attributes["objectClass"].push_back("organizationalUnit");
  people.attributes["ou"].push_back("People");
  return daemon.initialize() && daemon.backend() != nullptr && daemon.backend()->add(base) &&
         daemon.backend()->add(people);
}

bool testWriteEnforcement() {
  const std::string dir = schemaDir();
  if (dir.empty()) {
    return false;
  }
  LdapConfig config;
  config.listen_address = "127.0.0.1";
  config.ldap_port = 0;
  config.root_dn = "cn=admin,dc=example,dc=com";
  config.root_password = "secret";
  config.schema_dir = dir;
  LdapDaemon daemon(config);
  if (!seedBase(daemon) || !daemon.start()) {
    return false;
  }
  std::string error;
  cli::ClientOptions options;
  options.host = "127.0.0.1";
  options.port = daemon.boundPort();
  options.bind_dn = config.root_dn;
  options.password = config.root_password;
  auto client = cli::LdapClient::openBound(options, error);
  if (!client) {
    daemon.stop();
    return false;
  }
  DirectoryEntry missing;
  missing.dn = "uid=bad,ou=People,dc=example,dc=com";
  missing.attributes["objectClass"].push_back("inetOrgPerson");
  missing.attributes["cn"].push_back("Bad");
  missing.attributes["uid"].push_back("bad");
  const ResultCode rejected = client->add(missing);
  auto bob = validPerson();
  bob.dn = "uid=bob,ou=People,dc=example,dc=com";
  const ResultCode added = client->add(bob);
  const ResultCode mail =
      client->modify(bob.dn, {{ModifyOp::Replace, "mail", {"bob@example.com"}}});
  client->unbind();
  daemon.stop();
  return rejected == ResultCode::ObjectClassViolation && added == ResultCode::Success &&
         mail == ResultCode::Success;
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
  run("testParseNamesAndSyntax", testParseNamesAndSyntax);
  run("testProjectSchemas", testProjectSchemas);
  run("testValidateInetOrgPerson", testValidateInetOrgPerson);
  run("testMissingMust", testMissingMust);
  run("testUndefinedAttribute", testUndefinedAttribute);
  run("testNotAllowedAttribute", testNotAllowedAttribute);
  run("testPosixAndAdCompat", testPosixAndAdCompat);
  run("testTwoStructuralClasses", testTwoStructuralClasses);
  run("testSingleValue", testSingleValue);
  run("testWriteEnforcement", testWriteEnforcement);
  std::cout << "Schema tests: " << passed << "/" << total << std::endl;
  return passed == total ? 0 : 1;
}
