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

#include "Voxel.hpp"
#include <climits>
#include <cmath>
#include <functional>
#include <iostream>

// const Voxel INVALID(INFINITY, std::array<long, 3>({ LONG_MAX, LONG_MAX,
// LONG_MAX })); const Voxel EMPTY(0.0f, std::array<long, 3>{0, 0, 0});

Voxel::Voxel() { labels.reset(); }

Voxel::Voxel(float grayValue) : labels{} { this->grayValue = grayValue; }

// Getter
float Voxel::getGrayValue() const { return this->grayValue; }

float Voxel::getGradient() const { return this->gradient; }

std::bitset<Voxel::MAX_LABELS> Voxel::getLabels() const { return labels; }

int8_t Voxel::getFirstLabel() const {
  int8_t firstLabel = -1;
  for (int8_t i = 0; i < Voxel::MAX_LABELS; i++) {
    if (labels[i]) {
      firstLabel = i;
      break;
    }
  }
  return firstLabel;
}

bool Voxel::isDoubtful() const { return doubtful; }

uint8_t Voxel::getLabelCount() const {
  //    uint8_t count = 0;
  //    for (uint8_t i = 0; i < Voxel::MAX_LABELS; i++) {
  //        if (labels[i]) count++;
  //    }
  //    return count;
  return (uint8_t)labels.count();
}

std::vector<uint8_t> Voxel::getAllLabels() const {
  std::vector<uint8_t> allLabels;
  for (int8_t i = 0; i < Voxel::MAX_LABELS; i++) {
    if (labels[i]) {
      allLabels.push_back(i);
    }
  }
  return allLabels;
}

// Setter
void Voxel::addLabel(int8_t label) {
  if (label != -1) labels.set(label, true);
}

void Voxel::setLabels(std::bitset<Voxel::MAX_LABELS> newLabels) {
  labels = newLabels;
}

void Voxel::clearLabels() { labels.reset(); }

void Voxel::setDoubt(bool doubt) { doubtful = doubt; }

void Voxel::setTempLabels(std::bitset<Voxel::MAX_LABELS> newTempLabels) {
  tempLabels = newTempLabels;
}

void Voxel::applyLabelChanges() { labels = tempLabels; }

// other
void Voxel::computeGradient(vx::Array3<Voxel>& voxels, size_t x, size_t y,
                            size_t z) {
  float gradientX = 0.0f;
  float gradientY = 0.0f;
  float gradientZ = 0.0f;

  // Corners
  if (x + 1 < voxels.size<0>() && y + 1 < voxels.size<1>() &&
      z + 1 < voxels.size<2>()) {
    gradientX += voxels(x + 1, y + 1, z + 1).getGrayValue();
    gradientY += voxels(x + 1, y + 1, z + 1).getGrayValue();
    gradientZ += voxels(x + 1, y + 1, z + 1).getGrayValue();
  }

  if (x + 1 < voxels.size<0>() && y + 1 < voxels.size<1>() && z > 0) {
    gradientX += voxels(x + 1, y + 1, z - 1).getGrayValue();
    gradientY += voxels(x + 1, y + 1, z - 1).getGrayValue();
    gradientZ -= voxels(x + 1, y + 1, z - 1).getGrayValue();
  }

  if (x + 1 < voxels.size<0>() && y > 0 && z + 1 < voxels.size<2>()) {
    gradientX += voxels(x + 1, y - 1, z + 1).getGrayValue();
    gradientY -= voxels(x + 1, y - 1, z + 1).getGrayValue();
    gradientZ += voxels(x + 1, y - 1, z + 1).getGrayValue();
  }

  if (x + 1 < voxels.size<0>() && y > 0 && z > 0) {
    gradientX += voxels(x + 1, y - 1, z - 1).getGrayValue();
    gradientY -= voxels(x + 1, y - 1, z - 1).getGrayValue();
    gradientZ -= voxels(x + 1, y - 1, z - 1).getGrayValue();
  }

  if (x > 0 && y + 1 < voxels.size<1>() && z + 1 < voxels.size<2>()) {
    gradientX -= voxels(x - 1, y + 1, z + 1).getGrayValue();
    gradientY += voxels(x - 1, y + 1, z + 1).getGrayValue();
    gradientZ += voxels(x - 1, y + 1, z + 1).getGrayValue();
  }

  if (x > 0 && y + 1 < voxels.size<1>() && z > 0) {
    gradientX -= voxels(x - 1, y + 1, z - 1).getGrayValue();
    gradientY += voxels(x - 1, y + 1, z - 1).getGrayValue();
    gradientZ -= voxels(x - 1, y + 1, z - 1).getGrayValue();
  }

  if (x > 0 && y > 0 && z + 1 < voxels.size<2>()) {
    gradientX -= voxels(x - 1, y - 1, z + 1).getGrayValue();
    gradientY -= voxels(x - 1, y - 1, z + 1).getGrayValue();
    gradientZ += voxels(x - 1, y - 1, z + 1).getGrayValue();
  }

  if (x > 0 && y > 0 && z > 0) {
    gradientX -= voxels(x - 1, y - 1, z - 1).getGrayValue();
    gradientY -= voxels(x - 1, y - 1, z - 1).getGrayValue();
    gradientZ -= voxels(x - 1, y - 1, z - 1).getGrayValue();
  }

  // Edges
  if (x + 1 < voxels.size<0>() && y + 1 < voxels.size<1>()) {
    gradientX += voxels(x + 1, y + 1, z).getGrayValue() * 2;
    gradientY += voxels(x + 1, y + 1, z).getGrayValue() * 2;
  }

  if (x + 1 < voxels.size<0>() && y > 0) {
    gradientX += voxels(x + 1, y - 1, z).getGrayValue() * 2;
    gradientY -= voxels(x + 1, y - 1, z).getGrayValue() * 2;
  }

  if (x + 1 < voxels.size<0>() && z + 1 < voxels.size<2>()) {
    gradientX += voxels(x + 1, y, z + 1).getGrayValue() * 2;
    gradientZ += voxels(x + 1, y, z + 1).getGrayValue() * 2;
  }

  if (x + 1 < voxels.size<0>() && z > 0) {
    gradientX += voxels(x + 1, y, z - 1).getGrayValue() * 2;
    gradientZ -= voxels(x + 1, y, z - 1).getGrayValue() * 2;
  }

  if (x > 0 && y + 1 < voxels.size<1>()) {
    gradientX -= voxels(x - 1, y + 1, z).getGrayValue() * 2;
    gradientY += voxels(x - 1, y + 1, z).getGrayValue() * 2;
  }

  if (x > 0 && y > 0) {
    gradientX -= voxels(x - 1, y - 1, z).getGrayValue() * 2;
    gradientY -= voxels(x - 1, y - 1, z).getGrayValue() * 2;
  }

  if (x > 0 && z + 1 < voxels.size<2>()) {
    gradientX -= voxels(x - 1, y, z + 1).getGrayValue() * 2;
    gradientZ += voxels(x - 1, y, z + 1).getGrayValue() * 2;
  }

  if (x > 0 && z > 0) {
    gradientX -= voxels(x - 1, y, z - 1).getGrayValue() * 2;
    gradientZ -= voxels(x - 1, y, z - 1).getGrayValue() * 2;
  }

  if (y + 1 < voxels.size<1>() && z + 1 < voxels.size<2>()) {
    gradientY += voxels(x, y + 1, z + 1).getGrayValue() * 2;
    gradientZ += voxels(x, y + 1, z + 1).getGrayValue() * 2;
  }

  if (y + 1 < voxels.size<1>() && z > 0) {
    gradientY += voxels(x, y + 1, z - 1).getGrayValue() * 2;
    gradientZ -= voxels(x, y + 1, z - 1).getGrayValue() * 2;
  }

  if (y > 0 && z + 1 < voxels.size<2>()) {
    gradientY -= voxels(x, y - 1, z + 1).getGrayValue() * 2;
    gradientZ += voxels(x, y - 1, z + 1).getGrayValue() * 2;
  }

  if (y > 0 && z > 0) {
    gradientY -= voxels(x, y - 1, z - 1).getGrayValue() * 2;
    gradientZ -= voxels(x, y - 1, z - 1).getGrayValue() * 2;
  }

  // Direct Neighbours
  if (x + 1 < voxels.size<0>()) {
    gradientX += voxels(x + 1, y, z).getGrayValue() * 4;
  }

  if (x > 0) {
    gradientX -= voxels(x - 1, y, z).getGrayValue() * 4;
  }

  if (y + 1 < voxels.size<1>()) {
    gradientY += voxels(x, y + 1, z).getGrayValue() * 4;
  }

  if (y > 0) {
    gradientY -= voxels(x, y - 1, z).getGrayValue() * 4;
  }

  if (z + 1 < voxels.size<2>()) {
    gradientZ += voxels(x, y, z + 1).getGrayValue() * 4;
  }

  if (z > 0) {
    gradientZ -= voxels(x, y, z - 1).getGrayValue() * 4;
  }

  gradient = std::sqrt(gradientX * gradientX + gradientY * gradientY +
                       gradientZ * gradientZ);
}
