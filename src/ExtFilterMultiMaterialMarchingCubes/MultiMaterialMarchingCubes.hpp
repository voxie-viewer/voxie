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

#ifndef MULTIMATERIALMARCHINGCUBES_H
#define MULTIMATERIALMARCHINGCUBES_H

#include <VoxieClient/Array.hpp>
#include <bitset>
#include <list>

class MultiMaterialMarchingCubes {
 public:
  MultiMaterialMarchingCubes(
      vx::Array3<const uint8_t> inputVolume,
      vx::Array3<uint8_t>
          outputVolume);  // TODO: output is not a volume, it is a surface

  void run();

 private:
  vx::Array3<const uint8_t>& voxels;

  const size_t batchSize =
      3;  // number of layers per batch (should stay at 3 for now)

  [[gnu::unused]]  // TODO: Remove this?
  const size_t batchCount =
      1;  // number of parallel MMMC batches (should stay at 1 for now)

  const size_t
      batchCountMinor;  // number of simultaneously processed layers in xy- and
                        // yz-orientation (should stay uninitialized for now)

  /*
   * (TODO: List entries represent layers of cube faces (top, back and side))
   * Array [0], [1] and [2] represent squares oriented on xz-, xy- and yz-plane
   * Vector contains all layers of selected orientation on selected cube layer
   * Array2 contains one layer of calculated edge and vertex data
   * Pairs of bitsets store veteces and edges calculated via marching squares
   * (see marchingsquares.h for details)
   */
  /*std::list<*/ std::array<
      std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>>,
      3> /*>*/ squares;
  [[gnu::unused]]  // TODO: Remove this?
  size_t layerIndex = 0;

  // marching squares per layer
  vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>* marchingSquaresLayerXZ(
      size_t y);
  vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>* marchingSquaresLayerXY(
      size_t z, size_t yBegin, size_t yEnd);
  vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>* marchingSquaresLayerYZ(
      size_t x, size_t yBegin, size_t yEnd);

  // marching squares, all layers from begin to end per orientation, MUST NOT
  // run simultaneously
  std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
  marchingSquaresAxisY(size_t yBegin, size_t yEnd);
  std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
  marchingSquaresAxisZ(size_t yBegin, size_t yEnd);
  std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
  marchingSquaresAxisX(size_t yBegin, size_t yEnd);

  // similar structure can be used for marching cubes phase
};

#endif  // MULTIMATERIALMARCHINGCUBES_H
