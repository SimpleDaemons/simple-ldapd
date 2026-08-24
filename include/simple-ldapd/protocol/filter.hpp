/**
 * @file filter.hpp
 * @brief LDAP search filters (string and BER)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-ldapd/backend/backend.hpp"
#include "simple-ldapd/protocol/ber.hpp"
#include <string>
#include <vector>

namespace simple_ldapd {

enum class FilterType { And, Or, Not, Equality, Present, Substring, True, False };

struct FilterNode {
  FilterType type{FilterType::True};
  std::string attribute;
  std::string value;
  std::vector<FilterNode> children;
  std::string initial;
  std::vector<std::string> any;
  std::string final;
};

class SearchFilter {
public:
  static SearchFilter parse(const std::string &text);
  static SearchFilter present(const std::string &attribute);
  static SearchFilter equality(const std::string &attribute, const std::string &value);
  static SearchFilter all();

  const std::string &text() const { return text_; }
  bool valid() const { return valid_; }
  const FilterNode &root() const { return root_; }

  bool matches(const DirectoryEntry &entry) const;
  std::vector<uint8_t> encodeBer() const;
  static bool decodeBer(BerReader &reader, SearchFilter &out);

private:
  std::string text_;
  bool valid_{false};
  FilterNode root_{};
};

}  // namespace simple_ldapd
