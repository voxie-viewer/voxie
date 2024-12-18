/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Jpeg/HuffmanTable.hpp>

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Format.hpp>

#include <QtCore/QDebug>

namespace vx {
namespace jpeg {
// TODO: Move somewhere else?
struct VOXIEBACKEND_EXPORT HuffmanSymbolValue {
 public:
  HuffmanSymbol symbol;
  std::uint16_t value;

  HuffmanSymbolValue(HuffmanSymbol symbol, std::uint16_t value)
      : symbol(symbol), value(value) {}

  friend QDebug operator<<(QDebug debug, const HuffmanSymbolValue& val) {
    debug << vx::format("({0:02x}, {1:04x})", val.symbol, val.value)
                 .toUtf8()
                 .data();
    return debug;
  }
};

class VOXIEBACKEND_EXPORT HuffmanDecoder {
  const uint8_t* data_;
  size_t length_;

  uint8_t currentByte;
  uint8_t currentBitPos = 8;
  size_t pos = 0;

 public:
  HuffmanDecoder(const uint8_t* data, size_t length);
  ~HuffmanDecoder();

  const uint8_t* data() const { return data_; }
  size_t length() const { return length_; }

  bool readBit() {
    if (currentBitPos >= 8) {
      if (pos >= length())
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Reached EOF while reading JPEG data");
      currentByte = data()[pos];
      pos++;
      currentBitPos = 0;
    }
    // bool value = (currentByte & (1 << currentBitPos)) != 0;
    bool value = (currentByte & (1 << (7 - currentBitPos))) != 0;
    currentBitPos++;
    return value;
  }

  // TODO: Error checking?
  HuffmanSymbol decodeSymbol(HuffmanTable* table) {
    int bitCount = 1;
    std::uint16_t code = readBit();
    while (code > table->maxcode().at(bitCount)) {
      bitCount++;
      if (bitCount > 16) {
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Got too many bits in code word");
      }
      code = (code << 1) | readBit();
    }

    // qDebug() << bitCount << table->valptr().size() <<
    // table->mincode().size();
    auto val = table->valptr().at(bitCount);
    val = val + code - table->mincode().at(bitCount);
    // qDebug() << val << table->huffval().size();
    return table->huffval().at(val);
  }

  HuffmanSymbolValue readSymbolAndValue(HuffmanTable* table) {
    auto symbol = decodeSymbol(table);
    auto valueLen = symbol & 0x0f;

    std::uint16_t value = 0;
    for (int i = 0; i < valueLen; i++) value = (value << 1) | readBit();

    return HuffmanSymbolValue(symbol, value);
  }

  std::uint64_t remainingBits() {
    return (std::uint64_t)(length() - pos) * 8 + (8 - currentBitPos);
  }

  void assertAtEnd();

  void countCodewordsOneBlock(HuffmanTable* dcTable, HuffmanTable* acTable,
                              std::uint64_t* dcCounters,
                              std::size_t dcCountersSize,
                              std::uint64_t* acCounters,
                              std::size_t acCountersSize);
};
}  // namespace jpeg
}  // namespace vx
