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

#include "HuffmanTable.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtCore/QDebug>

vx::jpeg::HuffmanTable::HuffmanTable(
    const QList<QList<quint8>>& tableSpecification) {
  if (tableSpecification.size() != 16)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Error in huffman table: tableSpecification.size() != 16");

  huffsize_.resize(0);
  huffcode_.resize(0);
  ehufco_.resize(256, 0);
  ehufsi_.resize(256, 0);

  maxSymbol_ = 0;

  bits_.resize(0);
  huffval_.resize(0);

  bits_.push_back(0);

  mincode_.resize(0);
  maxcode_.resize(0);
  valptr_.resize(0);

  mincode_.push_back(0);
  maxcode_.push_back(0);
  valptr_.push_back(0);

  // See ISO/IEC 10918-1:1993(E) C.2
  // Generate_size_table (Figure C.1), Generate_code_table (Figure C.2),
  // Order_codes (Figure C.3)
  int currentLength = 1;
  quint16 currentCode = 0;
  for (const auto& values : tableSpecification) {
    if (currentCode & 0x8000)
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                          "Huffman table entries are too short");
    currentCode <<= 1;

    auto startPos = huffcode_.size();
    for (const auto& symbol : values) {
      huffval_.push_back(symbol);

      maxSymbol_ = std::max(maxSymbol_, symbol);

      huffsize_.push_back(currentLength);
      huffcode_.push_back(currentCode);

      ehufco_[symbol] = currentCode;
      ehufsi_[symbol] = currentLength;

      if (currentCode == 0xffff)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                            "Huffman table entries are too short");
      currentCode += 1;
    }

    currentLength++;

    // Decoder_tables (Figure F.15)
    size_t len = values.size();
    bits_.push_back(len);
    valptr_.push_back(startPos);
    if (len == 0) {
      mincode_.push_back(0);
      maxcode_.push_back(-1);
    } else {
      mincode_.push_back(huffcode_[startPos]);
      maxcode_.push_back(huffcode_[huffcode_.size() - 1]);
    }
  }
  invalidCode_ = currentCode;
}
vx::jpeg::HuffmanTable::~HuffmanTable() {}
