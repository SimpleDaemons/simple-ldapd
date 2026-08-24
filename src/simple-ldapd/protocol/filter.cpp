/**
 * @file filter.cpp
 * @brief LDAP search filter parse, match, and BER codec
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/protocol/filter.hpp"

#include "simple-ldapd/utils/dn.hpp"
#include <cctype>

namespace simple_ldapd {

namespace {

bool parseFilter(const std::string &text, size_t &pos, FilterNode &out);

void skipSpace(const std::string &text, size_t &pos) {
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
}

bool parseItem(const std::string &text, size_t &pos, FilterNode &out) {
  const size_t start = pos;
  while (pos < text.size() && text[pos] != '=' && text[pos] != ')' && text[pos] != '(') {
    ++pos;
  }
  if (pos >= text.size() || text[pos] != '=') {
    return false;
  }
  std::string attribute = text.substr(start, pos - start);
  ++pos;
  const size_t value_start = pos;
  while (pos < text.size() && text[pos] != ')') {
    ++pos;
  }
  std::string value = text.substr(value_start, pos - value_start);
  while (!attribute.empty() && std::isspace(static_cast<unsigned char>(attribute.back()))) {
    attribute.pop_back();
  }
  if (attribute.empty()) {
    return false;
  }
  if (value == "*") {
    out.type = FilterType::Present;
    out.attribute = attribute;
    out.value.clear();
    return true;
  }
  std::vector<std::string> parts;
  std::string current;
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '\\' && i + 1 < value.size()) {
      current.push_back(value[i]);
      current.push_back(value[++i]);
      continue;
    }
    if (value[i] == '*') {
      parts.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(value[i]);
  }
  parts.push_back(current);
  if (parts.size() == 1) {
    out.type = FilterType::Equality;
    out.attribute = attribute;
    out.value = value;
    return true;
  }
  out.type = FilterType::Substring;
  out.attribute = attribute;
  if (!parts.front().empty()) {
    out.initial = parts.front();
  }
  if (!parts.back().empty()) {
    out.final = parts.back();
  }
  for (size_t i = 1; i + 1 < parts.size(); ++i) {
    if (!parts[i].empty()) {
      out.any.push_back(parts[i]);
    }
  }
  return true;
}

bool parseList(const std::string &text, size_t &pos, FilterNode &out) {
  while (pos < text.size() && text[pos] == '(') {
    FilterNode child;
    if (!parseFilter(text, pos, child)) {
      return false;
    }
    out.children.push_back(child);
  }
  return !out.children.empty();
}

bool parseFilter(const std::string &text, size_t &pos, FilterNode &out) {
  skipSpace(text, pos);
  if (pos >= text.size() || text[pos] != '(') {
    return false;
  }
  ++pos;
  skipSpace(text, pos);
  if (pos >= text.size()) {
    return false;
  }
  if (text[pos] == '&') {
    ++pos;
    out.type = FilterType::And;
    if (!parseList(text, pos, out)) {
      return false;
    }
  } else if (text[pos] == '|') {
    ++pos;
    out.type = FilterType::Or;
    if (!parseList(text, pos, out)) {
      return false;
    }
  } else if (text[pos] == '!') {
    ++pos;
    out.type = FilterType::Not;
    FilterNode child;
    if (!parseFilter(text, pos, child)) {
      return false;
    }
    out.children.push_back(child);
  } else if (!parseItem(text, pos, out)) {
    return false;
  }
  skipSpace(text, pos);
  if (pos >= text.size() || text[pos] != ')') {
    return false;
  }
  ++pos;
  return true;
}

const std::vector<std::string> *findAttribute(const DirectoryEntry &entry,
                                              const std::string &name) {
  for (const auto &pair : entry.attributes) {
    if (iequals(pair.first, name)) {
      return &pair.second;
    }
  }
  return nullptr;
}

bool substringMatches(const std::string &haystack, const FilterNode &node) {
  const std::string hay = toLowerAscii(haystack);
  size_t pos = 0;
  if (!node.initial.empty()) {
    const std::string initial = toLowerAscii(node.initial);
    if (hay.size() < initial.size() || hay.compare(0, initial.size(), initial) != 0) {
      return false;
    }
    pos = initial.size();
  }
  for (const auto &part : node.any) {
    const std::string needle = toLowerAscii(part);
    const auto found = hay.find(needle, pos);
    if (found == std::string::npos) {
      return false;
    }
    pos = found + needle.size();
  }
  if (!node.final.empty()) {
    const std::string final_part = toLowerAscii(node.final);
    if (hay.size() < pos + final_part.size()) {
      return false;
    }
    if (hay.compare(hay.size() - final_part.size(), final_part.size(), final_part) != 0) {
      return false;
    }
  }
  return true;
}

bool nodeMatches(const FilterNode &node, const DirectoryEntry &entry) {
  switch (node.type) {
  case FilterType::True:
    return true;
  case FilterType::False:
    return false;
  case FilterType::And:
    for (const auto &child : node.children) {
      if (!nodeMatches(child, entry)) {
        return false;
      }
    }
    return true;
  case FilterType::Or:
    for (const auto &child : node.children) {
      if (nodeMatches(child, entry)) {
        return true;
      }
    }
    return false;
  case FilterType::Not:
    return node.children.size() == 1 && !nodeMatches(node.children.front(), entry);
  case FilterType::Present:
    return findAttribute(entry, node.attribute) != nullptr;
  case FilterType::Equality: {
    const auto *values = findAttribute(entry, node.attribute);
    if (values == nullptr) {
      return false;
    }
    for (const auto &value : *values) {
      if (iequals(value, node.value)) {
        return true;
      }
    }
    return false;
  }
  case FilterType::Substring: {
    const auto *values = findAttribute(entry, node.attribute);
    if (values == nullptr) {
      return false;
    }
    for (const auto &value : *values) {
      if (substringMatches(value, node)) {
        return true;
      }
    }
    return false;
  }
  }
  return false;
}

void encodeNode(BerWriter &writer, const FilterNode &node) {
  switch (node.type) {
  case FilterType::And:
  case FilterType::Or: {
    BerWriter inner;
    for (const auto &child : node.children) {
      encodeNode(inner, child);
    }
    writer.writeConstructed(
        node.type == FilterType::And ? kBerFilterAnd : kBerFilterOr, inner.bytes());
    break;
  }
  case FilterType::Not: {
    BerWriter inner;
    if (!node.children.empty()) {
      encodeNode(inner, node.children.front());
    }
    writer.writeConstructed(kBerFilterNot, inner.bytes());
    break;
  }
  case FilterType::Present:
    writer.writeOctetString(kBerFilterPresent, node.attribute);
    break;
  case FilterType::Equality: {
    BerWriter inner;
    inner.writeOctetString(node.attribute);
    inner.writeOctetString(node.value);
    writer.writeConstructed(kBerFilterEquality, inner.bytes());
    break;
  }
  case FilterType::Substring: {
    BerWriter inner;
    inner.writeOctetString(node.attribute);
    BerWriter parts;
    if (!node.initial.empty()) {
      parts.writeOctetString(kBerSubstringInitial, node.initial);
    }
    for (const auto &part : node.any) {
      parts.writeOctetString(kBerSubstringAny, part);
    }
    if (!node.final.empty()) {
      parts.writeOctetString(kBerSubstringFinal, node.final);
    }
    inner.writeConstructed(kBerSequence, parts.bytes());
    writer.writeConstructed(kBerFilterSubstrings, inner.bytes());
    break;
  }
  case FilterType::True:
    writer.writeOctetString(kBerFilterPresent, "objectClass");
    break;
  case FilterType::False: {
    BerWriter inner;
    FilterNode present;
    present.type = FilterType::Present;
    present.attribute = "objectClass";
    encodeNode(inner, present);
    writer.writeConstructed(kBerFilterNot, inner.bytes());
    break;
  }
  }
}

bool decodeNode(BerReader &reader, FilterNode &out) {
  const uint8_t tag = reader.peekTag();
  if (tag == kBerFilterAnd || tag == kBerFilterOr) {
    BerReader inner;
    if (!reader.readConstructed(tag, inner)) {
      return false;
    }
    out.type = tag == kBerFilterAnd ? FilterType::And : FilterType::Or;
    while (!inner.atEnd()) {
      FilterNode child;
      if (!decodeNode(inner, child)) {
        return false;
      }
      out.children.push_back(child);
    }
    return inner.ok();
  }
  if (tag == kBerFilterNot) {
    BerReader inner;
    if (!reader.readConstructed(tag, inner)) {
      return false;
    }
    out.type = FilterType::Not;
    FilterNode child;
    if (!decodeNode(inner, child)) {
      return false;
    }
    out.children.push_back(child);
    return inner.ok();
  }
  if (tag == kBerFilterPresent) {
    out.type = FilterType::Present;
    return reader.readOctetString(kBerFilterPresent, out.attribute);
  }
  if (tag == kBerFilterEquality) {
    BerReader inner;
    if (!reader.readConstructed(tag, inner)) {
      return false;
    }
    out.type = FilterType::Equality;
    return inner.readOctetString(out.attribute) && inner.readOctetString(out.value) &&
           inner.ok();
  }
  if (tag == kBerFilterSubstrings) {
    BerReader inner;
    if (!reader.readConstructed(tag, inner) || !inner.readOctetString(out.attribute)) {
      return false;
    }
    BerReader parts;
    if (!inner.readConstructed(kBerSequence, parts)) {
      return false;
    }
    out.type = FilterType::Substring;
    while (!parts.atEnd()) {
      const uint8_t part_tag = parts.peekTag();
      std::string component;
      if (part_tag == kBerSubstringInitial) {
        if (!parts.readOctetString(kBerSubstringInitial, component)) {
          return false;
        }
        out.initial = std::move(component);
      } else if (part_tag == kBerSubstringAny) {
        if (!parts.readOctetString(kBerSubstringAny, component)) {
          return false;
        }
        out.any.push_back(std::move(component));
      } else if (part_tag == kBerSubstringFinal) {
        if (!parts.readOctetString(kBerSubstringFinal, component)) {
          return false;
        }
        out.final = std::move(component);
      } else if (!parts.skip()) {
        return false;
      }
    }
    return parts.ok() && inner.ok();
  }
  return false;
}

}  // namespace

SearchFilter SearchFilter::parse(const std::string &text) {
  SearchFilter filter;
  filter.text_ = text;
  size_t pos = 0;
  filter.valid_ = parseFilter(text, pos, filter.root_);
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  if (pos != text.size()) {
    filter.valid_ = false;
  }
  return filter;
}

SearchFilter SearchFilter::present(const std::string &attribute) {
  SearchFilter filter;
  filter.text_ = "(" + attribute + "=*)";
  filter.valid_ = true;
  filter.root_.type = FilterType::Present;
  filter.root_.attribute = attribute;
  return filter;
}

SearchFilter SearchFilter::equality(const std::string &attribute,
                                    const std::string &value) {
  SearchFilter filter;
  filter.text_ = "(" + attribute + "=" + value + ")";
  filter.valid_ = true;
  filter.root_.type = FilterType::Equality;
  filter.root_.attribute = attribute;
  filter.root_.value = value;
  return filter;
}

SearchFilter SearchFilter::all() {
  SearchFilter filter;
  filter.text_ = "(objectClass=*)";
  filter.valid_ = true;
  filter.root_.type = FilterType::True;
  return filter;
}

bool SearchFilter::matches(const DirectoryEntry &entry) const {
  return valid_ && nodeMatches(root_, entry);
}

std::vector<uint8_t> SearchFilter::encodeBer() const {
  BerWriter writer;
  if (valid_) {
    encodeNode(writer, root_);
  } else {
    FilterNode present;
    present.type = FilterType::Present;
    present.attribute = "objectClass";
    encodeNode(writer, present);
  }
  return writer.take();
}

bool SearchFilter::decodeBer(BerReader &reader, SearchFilter &out) {
  out = SearchFilter{};
  out.valid_ = decodeNode(reader, out.root_);
  out.text_ = "(decoded)";
  return out.valid_ && reader.ok();
}

}  // namespace simple_ldapd
