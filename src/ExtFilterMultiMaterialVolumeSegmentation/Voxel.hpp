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

// Voxel is a 3d pixel that contains a gray value and other stuff that arent
// important rn

#pragma once
#include <algorithm>
#include <array>
#include <bitset>
#include <list>
#include <set>
#include <string>
#include <vector>

#include <VoxieClient/Array.hpp>

class Voxel {
 public:
  Voxel();
  // cosntructor with gray value
  Voxel(float);

  static const uint8_t MAX_LABELS = 32;

  // Getter
  float getGrayValue() const;
  float getGradient() const;
  std::bitset<MAX_LABELS> getLabels() const;
  int8_t getFirstLabel() const;
  uint8_t getLabelCount() const;
  std::vector<uint8_t> getAllLabels() const;
  bool isDoubtful() const;

  // Setter
  void addLabel(int8_t);
  void setLabels(std::bitset<MAX_LABELS>);
  void clearLabels();
  void setDoubt(bool);
  void setTempLabels(std::bitset<Voxel::MAX_LABELS> newLabels);
  void applyLabelChanges();

  // other
  void computeGradient(vx::Array3<Voxel>& voxels, size_t x, size_t y, size_t z);

 private:
  float grayValue;
  float gradient;
  std::bitset<MAX_LABELS> labels;
  std::bitset<MAX_LABELS> tempLabels;
  bool doubtful = false;
};
