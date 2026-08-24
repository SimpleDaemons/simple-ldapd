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
  if (value == "*") {
    out.type = FilterType::Present;
    out.attribute = attribute;
    out.value.clear();
  } else {
    out.type = FilterType::Equality;
    out.attribute = attribute;
    out.value = value;
  }
  return !attribute.empty();
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
  case FilterType::True:
    writer.writeOctetString(kBerFilterPresent, "objectClass");
    break;
  case FilterType::False: {
    BerWriter inner;
    encodeNode(inner, FilterNode{FilterType::Present, "objectClass", "", {}});
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
  encodeNode(writer, valid_ ? root_ : FilterNode{FilterType::Present, "objectClass", "", {}});
  return writer.take();
}

bool SearchFilter::decodeBer(BerReader &reader, SearchFilter &out) {
  out = SearchFilter{};
  out.valid_ = decodeNode(reader, out.root_);
  out.text_ = "(decoded)";
  return out.valid_ && reader.ok();
}

}  // namespace simple_ldapd
