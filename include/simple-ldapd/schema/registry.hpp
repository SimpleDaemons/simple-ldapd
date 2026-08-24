/**
 * @file registry.hpp
 * @brief LDAP schema registry
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/protocol/result_codes.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace simple_ldapd {

enum class ObjectClassKind { Abstract, Structural, Auxiliary };

struct SchemaAttribute {
  std::string oid;
  std::vector<std::string> names;
  std::string superior;
  std::string equality;
  std::string syntax;
  bool single_value{false};
};

struct SchemaObjectClass {
  std::string oid;
  std::vector<std::string> names;
  std::vector<std::string> superiors;
  ObjectClassKind kind{ObjectClassKind::Structural};
  std::vector<std::string> must;
  std::vector<std::string> may;
};

class SchemaRegistry {
public:
  bool loadFile(const std::string &path);
  bool loadDirectory(const std::string &directory);
  bool loaded() const { return !object_classes_.empty(); }
  const std::vector<SchemaAttribute> &attributes() const { return attributes_; }
  const std::vector<SchemaObjectClass> &objectClasses() const {
    return object_classes_;
  }
  bool hasAttribute(const std::string &name) const;
  bool hasObjectClass(const std::string &name) const;
  const SchemaAttribute *findAttribute(const std::string &name) const;
  const SchemaObjectClass *findObjectClass(const std::string &name) const;
  ResultCode validateEntry(const DirectoryEntry &entry) const;
  ResultCode validateEntry(const DirectoryEntry &entry, std::string &diagnostic) const;

private:
  void indexAttribute(size_t index);
  void indexObjectClass(size_t index);
  const SchemaAttribute *attributeSyntax(const SchemaAttribute &attribute) const;
  bool collectObjectClasses(const std::vector<std::string> &names,
                            std::vector<const SchemaObjectClass *> &classes,
                            std::string &diagnostic) const;

  std::vector<SchemaAttribute> attributes_;
  std::vector<SchemaObjectClass> object_classes_;
  std::unordered_map<std::string, size_t> attribute_index_;
  std::unordered_map<std::string, size_t> object_class_index_;
};

}  // namespace simple_ldapd
