/**
 * @file registry.cpp
 * @brief OpenLDAP-style schema parser and write-time validation
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/schema/registry.hpp"

#include "simple-ldapd/utils/dn.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

namespace simple_ldapd {

namespace {

std::string stripComments(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  bool in_quote = false;
  for (size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (ch == '\'') {
      in_quote = !in_quote;
      out.push_back(ch);
      continue;
    }
    if (!in_quote && ch == '#') {
      while (i < text.size() && text[i] != '\n') {
        ++i;
      }
      if (i < text.size()) {
        out.push_back('\n');
      }
      continue;
    }
    out.push_back(ch);
  }
  return out;
}

struct Parser {
  const std::string &text;
  size_t pos{0};

  bool atEnd() const { return pos >= text.size(); }

  void skip() {
    while (!atEnd() && std::isspace(static_cast<unsigned char>(text[pos]))) {
      ++pos;
    }
  }

  char peek() {
    skip();
    return atEnd() ? '\0' : text[pos];
  }

  bool consume(char expected) {
    if (peek() != expected) {
      return false;
    }
    ++pos;
    return true;
  }

  bool readQuoted(std::string &value) {
    skip();
    if (atEnd() || text[pos] != '\'') {
      return false;
    }
    ++pos;
    const size_t start = pos;
    while (!atEnd() && text[pos] != '\'') {
      ++pos;
    }
    if (atEnd()) {
      return false;
    }
    value = text.substr(start, pos - start);
    ++pos;
    return true;
  }

  bool readWord(std::string &value) {
    skip();
    if (atEnd()) {
      return false;
    }
    const unsigned char first = static_cast<unsigned char>(text[pos]);
    if (!(std::isalnum(first) || text[pos] == '.' || text[pos] == '-' || text[pos] == '{')) {
      return false;
    }
    const size_t start = pos;
    while (!atEnd()) {
      const char ch = text[pos];
      const unsigned char uc = static_cast<unsigned char>(ch);
      if (!(std::isalnum(uc) || ch == '.' || ch == '-' || ch == '{' || ch == '}' || ch == '_')) {
        break;
      }
      ++pos;
    }
    value = text.substr(start, pos - start);
    return !value.empty();
  }

  bool readName(std::string &value) {
    if (peek() == '\'') {
      return readQuoted(value);
    }
    return readWord(value);
  }

  bool readNameList(std::vector<std::string> &names) {
    names.clear();
    if (peek() == '(') {
      consume('(');
      while (peek() != ')' && peek() != '\0') {
        std::string name;
        if (!readName(name)) {
          return false;
        }
        names.push_back(std::move(name));
      }
      return consume(')');
    }
    std::string name;
    if (!readName(name)) {
      return false;
    }
    names.push_back(std::move(name));
    return true;
  }

  bool readDollarList(std::vector<std::string> &names) {
    names.clear();
    if (peek() == '(') {
      consume('(');
      while (peek() != ')' && peek() != '\0') {
        if (peek() == '$') {
          consume('$');
          continue;
        }
        std::string name;
        if (!readName(name)) {
          return false;
        }
        names.push_back(std::move(name));
      }
      return consume(')');
    }
    std::string name;
    if (!readName(name)) {
      return false;
    }
    names.push_back(std::move(name));
    return true;
  }
};

bool parseAttribute(Parser &parser, SchemaAttribute &attribute) {
  if (!parser.consume('(') || !parser.readWord(attribute.oid)) {
    return false;
  }
  while (parser.peek() != ')' && parser.peek() != '\0') {
    std::string key;
    if (!parser.readWord(key)) {
      return false;
    }
    if (iequals(key, "NAME")) {
      if (!parser.readNameList(attribute.names)) {
        return false;
      }
    } else if (iequals(key, "DESC")) {
      std::string unused;
      if (!parser.readQuoted(unused)) {
        return false;
      }
    } else if (iequals(key, "SUP")) {
      if (!parser.readName(attribute.superior)) {
        return false;
      }
    } else if (iequals(key, "EQUALITY")) {
      if (!parser.readWord(attribute.equality)) {
        return false;
      }
    } else if (iequals(key, "ORDERING") || iequals(key, "SUBSTR") || iequals(key, "USAGE")) {
      std::string unused;
      if (!parser.readWord(unused)) {
        return false;
      }
    } else if (iequals(key, "SYNTAX")) {
      if (!parser.readWord(attribute.syntax)) {
        return false;
      }
    } else if (iequals(key, "SINGLE-VALUE")) {
      attribute.single_value = true;
    } else if (iequals(key, "COLLECTIVE") || iequals(key, "NO-USER-MODIFICATION") ||
               iequals(key, "OBSOLETE")) {
      continue;
    } else {
      return false;
    }
  }
  return parser.consume(')') && !attribute.names.empty();
}

bool parseObjectClass(Parser &parser, SchemaObjectClass &object_class) {
  if (!parser.consume('(') || !parser.readWord(object_class.oid)) {
    return false;
  }
  while (parser.peek() != ')' && parser.peek() != '\0') {
    std::string key;
    if (!parser.readWord(key)) {
      return false;
    }
    if (iequals(key, "NAME")) {
      if (!parser.readNameList(object_class.names)) {
        return false;
      }
    } else if (iequals(key, "DESC")) {
      std::string unused;
      if (!parser.readQuoted(unused)) {
        return false;
      }
    } else if (iequals(key, "SUP")) {
      if (!parser.readDollarList(object_class.superiors)) {
        return false;
      }
    } else if (iequals(key, "MUST")) {
      if (!parser.readDollarList(object_class.must)) {
        return false;
      }
    } else if (iequals(key, "MAY")) {
      if (!parser.readDollarList(object_class.may)) {
        return false;
      }
    } else if (iequals(key, "ABSTRACT")) {
      object_class.kind = ObjectClassKind::Abstract;
    } else if (iequals(key, "STRUCTURAL")) {
      object_class.kind = ObjectClassKind::Structural;
    } else if (iequals(key, "AUXILIARY")) {
      object_class.kind = ObjectClassKind::Auxiliary;
    } else if (iequals(key, "OBSOLETE")) {
      continue;
    } else {
      return false;
    }
  }
  return parser.consume(')') && !object_class.names.empty();
}

const std::vector<std::string> *valuesNamed(const DirectoryEntry &entry,
                                            const std::string &name) {
  for (const auto &pair : entry.attributes) {
    if (iequals(pair.first, name)) {
      return &pair.second;
    }
  }
  return nullptr;
}

bool hasNonEmpty(const DirectoryEntry &entry, const std::string &name) {
  const auto *values = valuesNamed(entry, name);
  if (values == nullptr) {
    return false;
  }
  return std::any_of(values->begin(), values->end(),
                     [](const std::string &value) { return !value.empty(); });
}

std::string syntaxOid(const std::string &syntax) {
  const auto brace = syntax.find('{');
  return brace == std::string::npos ? syntax : syntax.substr(0, brace);
}

bool isDigits(const std::string &value, size_t start) {
  if (start >= value.size()) {
    return false;
  }
  for (size_t i = start; i < value.size(); ++i) {
    if (std::isdigit(static_cast<unsigned char>(value[i])) == 0) {
      return false;
    }
  }
  return true;
}

bool valueMatchesSyntax(const std::string &syntax, const std::string &value) {
  const std::string oid = syntaxOid(syntax);
  if (oid == "1.3.6.1.4.1.1466.115.121.1.27") {
    const size_t start = (!value.empty() && value.front() == '-') ? 1 : 0;
    return isDigits(value, start);
  }
  if (oid == "1.3.6.1.4.1.1466.115.121.1.7") {
    return value == "TRUE" || value == "FALSE";
  }
  if (oid == "1.3.6.1.4.1.1466.115.121.1.12") {
    return value.find('=') != std::string::npos;
  }
  if (oid == "1.3.6.1.4.1.1466.115.121.1.38") {
    return !value.empty() && value.find("..") == std::string::npos &&
           std::all_of(value.begin(), value.end(), [](unsigned char ch) {
             return std::isdigit(ch) != 0 || ch == '.';
           });
  }
  if (oid == "1.3.6.1.4.1.1466.115.121.1.26") {
    return std::all_of(value.begin(), value.end(),
                       [](unsigned char ch) { return ch <= 127; });
  }
  return true;
}

std::vector<std::string> schemaFiles(const std::string &directory) {
  std::vector<std::string> files;
#ifdef _WIN32
  const std::string pattern = directory + "\\*.schema";
  WIN32_FIND_DATAA data;
  HANDLE handle = FindFirstFileA(pattern.c_str(), &data);
  if (handle == INVALID_HANDLE_VALUE) {
    return files;
  }
  do {
    files.push_back(directory + "\\" + data.cFileName);
  } while (FindNextFileA(handle, &data));
  FindClose(handle);
#else
  DIR *dir = opendir(directory.c_str());
  if (dir == nullptr) {
    return files;
  }
  while (dirent *entry = readdir(dir)) {
    const std::string name = entry->d_name;
    if (name.size() > 7 && name.compare(name.size() - 7, 7, ".schema") == 0) {
      files.push_back(directory + "/" + name);
    }
  }
  closedir(dir);
#endif
  std::sort(files.begin(), files.end());
  return files;
}

}  // namespace

void SchemaRegistry::indexAttribute(size_t index) {
  for (const auto &name : attributes_[index].names) {
    attribute_index_[toLowerAscii(name)] = index;
  }
}

void SchemaRegistry::indexObjectClass(size_t index) {
  for (const auto &name : object_classes_[index].names) {
    object_class_index_[toLowerAscii(name)] = index;
  }
}

bool SchemaRegistry::loadFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string text = stripComments(buffer.str());
  Parser parser{text, 0};
  bool ok = true;
  while (!parser.atEnd()) {
    parser.skip();
    if (parser.atEnd()) {
      break;
    }
    std::string keyword;
    if (!parser.readWord(keyword)) {
      ++parser.pos;
      continue;
    }
    if (iequals(keyword, "attributetype") || iequals(keyword, "attributetypes")) {
      SchemaAttribute attribute;
      if (!parseAttribute(parser, attribute)) {
        ok = false;
        break;
      }
      attributes_.push_back(std::move(attribute));
      indexAttribute(attributes_.size() - 1);
    } else if (iequals(keyword, "objectclass") || iequals(keyword, "objectclasses")) {
      SchemaObjectClass object_class;
      if (!parseObjectClass(parser, object_class)) {
        ok = false;
        break;
      }
      object_classes_.push_back(std::move(object_class));
      indexObjectClass(object_classes_.size() - 1);
    }
  }
  return ok;
}

bool SchemaRegistry::loadDirectory(const std::string &directory) {
  const auto files = schemaFiles(directory);
  if (files.empty()) {
    return false;
  }
  bool ok = true;
  for (const auto &path : files) {
    ok = loadFile(path) && ok;
  }
  return ok;
}

const SchemaAttribute *SchemaRegistry::findAttribute(const std::string &name) const {
  const auto it = attribute_index_.find(toLowerAscii(name));
  if (it == attribute_index_.end()) {
    return nullptr;
  }
  return &attributes_[it->second];
}

const SchemaObjectClass *SchemaRegistry::findObjectClass(const std::string &name) const {
  const auto it = object_class_index_.find(toLowerAscii(name));
  if (it == object_class_index_.end()) {
    return nullptr;
  }
  return &object_classes_[it->second];
}

bool SchemaRegistry::hasAttribute(const std::string &name) const {
  return findAttribute(name) != nullptr;
}

bool SchemaRegistry::hasObjectClass(const std::string &name) const {
  return findObjectClass(name) != nullptr;
}

const SchemaAttribute *SchemaRegistry::attributeSyntax(const SchemaAttribute &attribute) const {
  const SchemaAttribute *current = &attribute;
  int depth = 0;
  while (current != nullptr && current->syntax.empty() && !current->superior.empty() &&
         depth < 32) {
    current = findAttribute(current->superior);
    ++depth;
  }
  return current;
}

bool SchemaRegistry::collectObjectClasses(const std::vector<std::string> &names,
                                          std::vector<const SchemaObjectClass *> &classes,
                                          std::string &diagnostic) const {
  std::vector<std::string> pending = names;
  std::set<std::string> seen;
  while (!pending.empty()) {
    const std::string name = pending.back();
    pending.pop_back();
    const std::string key = toLowerAscii(name);
    if (!seen.insert(key).second) {
      continue;
    }
    const SchemaObjectClass *object_class = findObjectClass(name);
    if (object_class == nullptr) {
      diagnostic = "unknown object class " + name;
      return false;
    }
    classes.push_back(object_class);
    for (const auto &superior : object_class->superiors) {
      pending.push_back(superior);
    }
  }
  return true;
}

ResultCode SchemaRegistry::validateEntry(const DirectoryEntry &entry) const {
  std::string diagnostic;
  return validateEntry(entry, diagnostic);
}

ResultCode SchemaRegistry::validateEntry(const DirectoryEntry &entry,
                                         std::string &diagnostic) const {
  diagnostic.clear();
  if (!loaded()) {
    return ResultCode::Success;
  }
  const auto *object_class_values = valuesNamed(entry, "objectClass");
  if (object_class_values == nullptr || object_class_values->empty()) {
    diagnostic = "objectClass is required";
    return ResultCode::ObjectClassViolation;
  }
  std::vector<const SchemaObjectClass *> classes;
  if (!collectObjectClasses(*object_class_values, classes, diagnostic)) {
    return ResultCode::ObjectClassViolation;
  }

  std::set<std::string> structural;
  std::set<std::string> structural_superiors;
  std::set<std::string> allowed;
  std::set<std::string> required;
  allowed.insert("objectclass");
  for (const SchemaObjectClass *object_class : classes) {
    const std::string primary = toLowerAscii(object_class->names.front());
    if (object_class->kind == ObjectClassKind::Structural) {
      structural.insert(primary);
      for (const auto &superior : object_class->superiors) {
        const SchemaObjectClass *parent = findObjectClass(superior);
        if (parent != nullptr && parent->kind == ObjectClassKind::Structural) {
          structural_superiors.insert(toLowerAscii(parent->names.front()));
        }
      }
    }
    for (const auto &name : object_class->must) {
      required.insert(toLowerAscii(name));
      allowed.insert(toLowerAscii(name));
    }
    for (const auto &name : object_class->may) {
      allowed.insert(toLowerAscii(name));
    }
  }
  std::set<std::string> leaves;
  for (const auto &name : structural) {
    if (structural_superiors.count(name) == 0) {
      leaves.insert(name);
    }
  }
  if (leaves.size() != 1) {
    diagnostic = "entry must have exactly one structural object class";
    return ResultCode::ObjectClassViolation;
  }
  for (const auto &name : required) {
    if (!hasNonEmpty(entry, name)) {
      diagnostic = "missing required attribute " + name;
      return ResultCode::ObjectClassViolation;
    }
  }
  for (const auto &pair : entry.attributes) {
    const std::string key = toLowerAscii(pair.first);
    if (key == "objectclass") {
      continue;
    }
    const SchemaAttribute *attribute = findAttribute(pair.first);
    if (attribute == nullptr) {
      diagnostic = "undefined attribute type " + pair.first;
      return ResultCode::UndefinedAttributeType;
    }
    if (allowed.count(key) == 0) {
      diagnostic = "attribute " + pair.first + " is not allowed by object class";
      return ResultCode::ObjectClassViolation;
    }
    if (attribute->single_value && pair.second.size() > 1) {
      diagnostic = "attribute " + pair.first + " is single-valued";
      return ResultCode::ConstraintViolation;
    }
    const SchemaAttribute *syntax_source = attributeSyntax(*attribute);
    const std::string &syntax =
        syntax_source != nullptr ? syntax_source->syntax : attribute->syntax;
    if (!syntax.empty()) {
      for (const auto &value : pair.second) {
        if (!valueMatchesSyntax(syntax, value)) {
          diagnostic = "invalid syntax for " + pair.first;
          return ResultCode::InvalidAttributeSyntax;
        }
      }
    }
  }
  return ResultCode::Success;
}

}  // namespace simple_ldapd
