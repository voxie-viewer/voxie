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

#include "HuffmanDecoder.hpp"

vx::jpeg::HuffmanDecoder::HuffmanDecoder(const uint8_t* data, size_t length)
    : data_(data), length_(length) {}
vx::jpeg::HuffmanDecoder::~HuffmanDecoder() {}

void vx::jpeg::HuffmanDecoder::assertAtEnd() {
  auto padding = this->remainingBits();
  // qDebug() << "rem" << remainingBits;
  if (padding >= 8) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        vx::format("Got too many bits left after decoding: {}", padding));
  }
  for (size_t i = 0; i < padding; i++) {
    auto bit = this->readBit();
    if (!bit)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          vx::format("Got zero padding bit at padding bit {} of {}", i,
                     padding));
  }
}

static inline void addOne(vx::jpeg::HuffmanTable* table,
                          std::uint64_t* counters,
                          vx::jpeg::HuffmanSymbol symbol) {
  if (symbol >= table->maxSymbol())
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "symbol >= table->maxSymbol()");
  if (counters) {
    if (counters[symbol] + 1 == 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Counter overflow");
    counters[symbol] += 1;
  }
}

void vx::jpeg::HuffmanDecoder::countCodewordsOneBlock(
    HuffmanTable* dcTable, HuffmanTable* acTable, std::uint64_t* dcCounters,
    std::size_t dcCountersSize, std::uint64_t* acCounters,
    std::size_t acCountersSize) {
  // TODO: Move this checks further out?
  if (dcCounters && dcCountersSize < dcTable->maxSymbol())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "dcCountersSize < dcTable->maxSymbol()");
  if (acCounters && acCountersSize < acTable->maxSymbol())
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "acCountersSize < acTable->maxSymbol()");

  // DC
  auto dcVal = this->readSymbolAndValue(dcTable);
  // qDebug() << "DC" << dcVal;
  addOne(dcTable, dcCounters, dcVal.symbol);

  // AC
  int position = 1;
  for (;;) {
    if (position >= 64) {
      if (position == 64) break;
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got too much AC data in JPEG block");
    }

    auto acVal = this->readSymbolAndValue(acTable);
    // qDebug() << position << acVal;
    addOne(acTable, acCounters, acVal.symbol);

    // EOB
    if (acVal.symbol == 0) break;

    position += 1 + (acVal.symbol >> 4);
  }
}
