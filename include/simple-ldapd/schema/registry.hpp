/**
 * @file registry.hpp
 * @brief LDAP schema registry
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

namespace simple_ldapd {

struct SchemaAttribute {
  std::string oid;
  std::string name;
};

struct SchemaObjectClass {
  std::string oid;
  std::string name;
};

class SchemaRegistry {
public:
  bool loadFile(const std::string &path);
  bool loadDirectory(const std::string &directory);
  const std::vector<SchemaAttribute> &attributes() const { return attributes_; }
  const std::vector<SchemaObjectClass> &objectClasses() const {
    return object_classes_;
  }
  bool hasAttribute(const std::string &name) const;
  bool hasObjectClass(const std::string &name) const;

private:
  std::vector<SchemaAttribute> attributes_;
  std::vector<SchemaObjectClass> object_classes_;
};

}  // namespace simple_ldapd
