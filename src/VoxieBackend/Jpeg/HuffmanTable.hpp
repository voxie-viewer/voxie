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

#include <QtCore/QList>

#include <vector>

namespace vx {
namespace jpeg {
using HuffmanSymbol = std::uint8_t;

class VOXIEBACKEND_EXPORT HuffmanTable {
  QList<QList<quint8>> tableSpecification_;

  // See ISO/IEC 10918-1:1993(E) C.2
  std::vector<std::uint8_t> huffsize_;
  std::vector<std::uint16_t> huffcode_;
  std::uint16_t invalidCode_;
  // Maps a symbol to the code word data
  std::vector<std::uint16_t> ehufco_;
  // Maps a symbol to the code word length
  std::vector<std::uint8_t> ehufsi_;

  std::uint8_t maxSymbol_;

  std::vector<int> bits_;
  std::vector<HuffmanSymbol> huffval_;

  std::vector<int> mincode_;
  std::vector<int> maxcode_;
  std::vector<int> valptr_;

 public:
  HuffmanTable(const QList<QList<quint8>>& tableSpecification);
  ~HuffmanTable();

  const QList<QList<quint8>>& tableSpecification() {
    return tableSpecification_;
  }

  const std::vector<std::uint8_t>& huffsize() { return huffsize_; }
  const std::vector<std::uint16_t>& huffcode() { return huffcode_; }
  std::uint16_t invalidCode() { return invalidCode_; }
  const std::vector<std::uint16_t>& ehufco() { return ehufco_; }
  const std::vector<std::uint8_t>& ehufsi() { return ehufsi_; }

  std::uint8_t maxSymbol() { return maxSymbol_; }

  const std::vector<int>& bits() { return bits_; }
  const std::vector<HuffmanSymbol>& huffval() { return huffval_; }

  const std::vector<int>& mincode() { return mincode_; }
  const std::vector<int>& maxcode() { return maxcode_; }
  const std::vector<int>& valptr() { return valptr_; }
};
}  // namespace jpeg
}  // namespace vx
