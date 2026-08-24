/**
 * @file ber.cpp
 * @brief BER reader and writer
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-ldapd/protocol/ber.hpp"

namespace simple_ldapd {

void BerWriter::writeByte(uint8_t value) { bytes_.push_back(value); }

void BerWriter::writeBytes(const uint8_t *data, size_t size) {
  bytes_.insert(bytes_.end(), data, data + size);
}

std::vector<uint8_t> BerWriter::encodeLength(size_t length) const {
  std::vector<uint8_t> encoded;
  if (length < 128) {
    encoded.push_back(static_cast<uint8_t>(length));
    return encoded;
  }
  std::vector<uint8_t> raw;
  size_t value = length;
  while (value > 0) {
    raw.push_back(static_cast<uint8_t>(value & 0xff));
    value >>= 8;
  }
  encoded.push_back(static_cast<uint8_t>(0x80 | raw.size()));
  encoded.insert(encoded.end(), raw.rbegin(), raw.rend());
  return encoded;
}

void BerWriter::writeTagLength(uint8_t tag, size_t length) {
  writeByte(tag);
  const auto encoded = encodeLength(length);
  writeBytes(encoded.data(), encoded.size());
}

std::vector<uint8_t> BerWriter::encodeInteger(int64_t value) const {
  std::vector<uint8_t> raw;
  uint64_t bits = static_cast<uint64_t>(value);
  for (int i = 0; i < 8; ++i) {
    raw.push_back(static_cast<uint8_t>((bits >> (56 - i * 8)) & 0xff));
  }
  size_t start = 0;
  while (start + 1 < raw.size()) {
    const bool sign_extend_zero = raw[start] == 0x00 && (raw[start + 1] & 0x80) == 0;
    const bool sign_extend_ones = raw[start] == 0xff && (raw[start + 1] & 0x80) != 0;
    if (!sign_extend_zero && !sign_extend_ones) {
      break;
    }
    ++start;
  }
  return std::vector<uint8_t>(raw.begin() + static_cast<std::ptrdiff_t>(start), raw.end());
}

void BerWriter::writeInteger(int64_t value) {
  const auto encoded = encodeInteger(value);
  writeTagLength(kBerInteger, encoded.size());
  writeBytes(encoded.data(), encoded.size());
}

void BerWriter::writeEnumerated(int value) {
  const auto encoded = encodeInteger(value);
  writeTagLength(kBerEnumerated, encoded.size());
  writeBytes(encoded.data(), encoded.size());
}

void BerWriter::writeBoolean(bool value) {
  writeTagLength(kBerBoolean, 1);
  writeByte(value ? 0xff : 0x00);
}

void BerWriter::writeOctetString(const std::string &value) {
  writeOctetString(kBerOctetString, value);
}

void BerWriter::writeOctetString(uint8_t tag, const std::string &value) {
  writeTagLength(tag, value.size());
  if (!value.empty()) {
    writeBytes(reinterpret_cast<const uint8_t *>(value.data()), value.size());
  }
}

void BerWriter::writeNull(uint8_t tag) { writeTagLength(tag, 0); }

void BerWriter::writeConstructed(uint8_t tag, const std::vector<uint8_t> &inner) {
  writeTagLength(tag, inner.size());
  if (!inner.empty()) {
    writeBytes(inner.data(), inner.size());
  }
}

std::vector<uint8_t> BerWriter::take() { return std::move(bytes_); }

BerReader::BerReader(const std::vector<uint8_t> &data) : data_(&data) {}

bool BerReader::atEnd() const {
  return !ok_ || data_ == nullptr || offset_ >= data_->size();
}

uint8_t BerReader::peekTag() const {
  if (!ok_ || data_ == nullptr || offset_ >= data_->size()) {
    return 0;
  }
  return (*data_)[offset_];
}

bool BerReader::consume(size_t n, const uint8_t *&ptr) {
  if (!ok_ || data_ == nullptr || offset_ + n > data_->size()) {
    ok_ = false;
    return false;
  }
  ptr = data_->data() + offset_;
  offset_ += n;
  return true;
}

bool BerReader::readLength(size_t &length) {
  const uint8_t *ptr = nullptr;
  if (!consume(1, ptr)) {
    return false;
  }
  const uint8_t first = ptr[0];
  if ((first & 0x80) == 0) {
    length = first;
    return true;
  }
  const size_t count = first & 0x7f;
  if (count == 0 || count > 4) {
    ok_ = false;
    return false;
  }
  if (!consume(count, ptr)) {
    return false;
  }
  length = 0;
  for (size_t i = 0; i < count; ++i) {
    length = (length << 8) | ptr[i];
  }
  return true;
}

bool BerReader::readTagLength(uint8_t &tag, size_t &length) {
  const uint8_t *ptr = nullptr;
  if (!consume(1, ptr)) {
    return false;
  }
  tag = ptr[0];
  return readLength(length);
}

bool BerReader::readConstructed(uint8_t tag, BerReader &inner) {
  uint8_t actual = 0;
  size_t length = 0;
  if (!readTagLength(actual, length) || actual != tag) {
    ok_ = false;
    return false;
  }
  if (data_ == nullptr || offset_ + length > data_->size()) {
    ok_ = false;
    return false;
  }
  inner.owned_.assign(data_->begin() + static_cast<std::ptrdiff_t>(offset_),
                      data_->begin() + static_cast<std::ptrdiff_t>(offset_ + length));
  inner.data_ = &inner.owned_;
  inner.offset_ = 0;
  inner.ok_ = true;
  offset_ += length;
  return true;
}

bool BerReader::readInteger(int64_t &value) {
  uint8_t tag = 0;
  size_t length = 0;
  if (!readTagLength(tag, length) || tag != kBerInteger || length == 0 || length > 8) {
    ok_ = false;
    return false;
  }
  const uint8_t *ptr = nullptr;
  if (!consume(length, ptr)) {
    return false;
  }
  const bool negative = (ptr[0] & 0x80) != 0;
  uint64_t bits = negative ? ~0ULL : 0;
  for (size_t i = 0; i < length; ++i) {
    bits = (bits << 8) | ptr[i];
  }
  value = static_cast<int64_t>(bits);
  return true;
}

bool BerReader::readEnumerated(int &value) {
  uint8_t tag = 0;
  size_t length = 0;
  if (!readTagLength(tag, length) || tag != kBerEnumerated || length == 0 ||
      length > 8) {
    ok_ = false;
    return false;
  }
  const uint8_t *ptr = nullptr;
  if (!consume(length, ptr)) {
    return false;
  }
  int64_t parsed = 0;
  for (size_t i = 0; i < length; ++i) {
    parsed = (parsed << 8) | ptr[i];
  }
  value = static_cast<int>(parsed);
  return true;
}

bool BerReader::readBoolean(bool &value) {
  uint8_t tag = 0;
  size_t length = 0;
  if (!readTagLength(tag, length) || tag != kBerBoolean || length != 1) {
    ok_ = false;
    return false;
  }
  const uint8_t *ptr = nullptr;
  if (!consume(length, ptr)) {
    return false;
  }
  value = ptr[0] != 0;
  return true;
}

bool BerReader::readOctetString(std::string &value) {
  return readOctetString(kBerOctetString, value);
}

bool BerReader::readOctetString(uint8_t tag, std::string &value) {
  uint8_t actual = 0;
  size_t length = 0;
  if (!readTagLength(actual, length) || actual != tag) {
    ok_ = false;
    return false;
  }
  if (length == 0) {
    value.clear();
    return true;
  }
  const uint8_t *ptr = nullptr;
  if (!consume(length, ptr)) {
    return false;
  }
  value.assign(reinterpret_cast<const char *>(ptr), length);
  return true;
}

bool BerReader::skip() {
  uint8_t tag = 0;
  size_t length = 0;
  if (!readTagLength(tag, length)) {
    return false;
  }
  const uint8_t *ptr = nullptr;
  return consume(length, ptr);
}

}  // namespace simple_ldapd
