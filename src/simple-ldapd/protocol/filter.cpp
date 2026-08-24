/**
 * @file filter.cpp
 * @brief Search filter stub
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/protocol/filter.hpp"

namespace simple_ldapd {

SearchFilter SearchFilter::parse(const std::string &text) {
  SearchFilter filter;
  filter.text_ = text;
  filter.valid_ = !text.empty() && text.front() == '(' && text.back() == ')';
  return filter;
}

const std::string &SearchFilter::text() const { return text_; }

bool SearchFilter::valid() const { return valid_; }

}  // namespace simple_ldapd
