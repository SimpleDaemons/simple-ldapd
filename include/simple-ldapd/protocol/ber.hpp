/**
 * @file ber.hpp
 * @brief BER reader and writer for LDAPv3 (X.690 subset)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace simple_ldapd {

inline constexpr uint8_t kBerSequence = 0x30;
inline constexpr uint8_t kBerSet = 0x31;
inline constexpr uint8_t kBerBoolean = 0x01;
inline constexpr uint8_t kBerInteger = 0x02;
inline constexpr uint8_t kBerOctetString = 0x04;
inline constexpr uint8_t kBerEnumerated = 0x0A;
inline constexpr uint8_t kBerNull = 0x05;
inline constexpr uint8_t kBerBindRequest = 0x60;
inline constexpr uint8_t kBerBindResponse = 0x61;
inline constexpr uint8_t kBerUnbindRequest = 0x42;
inline constexpr uint8_t kBerSearchRequest = 0x63;
inline constexpr uint8_t kBerSearchResultEntry = 0x64;
inline constexpr uint8_t kBerSearchResultDone = 0x65;
inline constexpr uint8_t kBerModifyRequest = 0x66;
inline constexpr uint8_t kBerModifyResponse = 0x67;
inline constexpr uint8_t kBerAddRequest = 0x68;
inline constexpr uint8_t kBerAddResponse = 0x69;
inline constexpr uint8_t kBerDelRequest = 0x4A;
inline constexpr uint8_t kBerDelResponse = 0x6B;
inline constexpr uint8_t kBerModifyDNRequest = 0x6C;
inline constexpr uint8_t kBerModifyDNResponse = 0x6D;
inline constexpr uint8_t kBerCompareRequest = 0x6E;
inline constexpr uint8_t kBerCompareResponse = 0x6F;
inline constexpr uint8_t kBerExtendedRequest = 0x77;
inline constexpr uint8_t kBerExtendedResponse = 0x78;
inline constexpr uint8_t kBerSimpleAuth = 0x80;
inline constexpr uint8_t kBerContext1 = 0x81;
inline constexpr uint8_t kBerContext2 = 0x82;
inline constexpr uint8_t kBerContext10 = 0x8A;
inline constexpr uint8_t kBerContext11 = 0x8B;
inline constexpr uint8_t kBerControls = 0xA0;
inline constexpr uint8_t kBerServerSaslCreds = 0x87;
inline constexpr uint8_t kBerSaslAuth = 0xA3;
inline constexpr uint8_t kBerFilterAnd = 0xA0;
inline constexpr uint8_t kBerFilterOr = 0xA1;
inline constexpr uint8_t kBerFilterNot = 0xA2;
inline constexpr uint8_t kBerFilterEquality = 0xA3;
inline constexpr uint8_t kBerFilterSubstrings = 0xA4;
inline constexpr uint8_t kBerFilterPresent = 0x87;
inline constexpr uint8_t kBerSubstringInitial = 0x80;
inline constexpr uint8_t kBerSubstringAny = 0x81;
inline constexpr uint8_t kBerSubstringFinal = 0x82;

class BerWriter {
public:
  void writeByte(uint8_t value);
  void writeBytes(const uint8_t *data, size_t size);
  void writeTagLength(uint8_t tag, size_t length);
  void writeInteger(int64_t value);
  void writeEnumerated(int value);
  void writeBoolean(bool value);
  void writeOctetString(const std::string &value);
  void writeOctetString(uint8_t tag, const std::string &value);
  void writeNull(uint8_t tag = kBerNull);
  void writeConstructed(uint8_t tag, const std::vector<uint8_t> &inner);
  const std::vector<uint8_t> &bytes() const { return bytes_; }
  std::vector<uint8_t> take();

private:
  std::vector<uint8_t> encodeLength(size_t length) const;
  std::vector<uint8_t> encodeInteger(int64_t value) const;
  std::vector<uint8_t> bytes_;
};

class BerReader {
public:
  BerReader() = default;
  explicit BerReader(const std::vector<uint8_t> &data);

  bool ok() const { return ok_; }
  bool atEnd() const;
  uint8_t peekTag() const;
  bool readTagLength(uint8_t &tag, size_t &length);
  bool readInteger(int64_t &value);
  bool readEnumerated(int &value);
  bool readBoolean(bool &value);
  bool readOctetString(std::string &value);
  bool readOctetString(uint8_t tag, std::string &value);
  bool readConstructed(uint8_t tag, BerReader &inner);
  bool skip();

private:
  bool readLength(size_t &length);
  bool consume(size_t n, const uint8_t *&ptr);
  std::vector<uint8_t> owned_;
  const std::vector<uint8_t> *data_{nullptr};
  size_t offset_{0};
  bool ok_{true};
};

}  // namespace simple_ldapd
