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

#ifndef MARCHINGCUBES_H
#define MARCHINGCUBES_H

#include <array>
#include <bitset>
#include <functional>
#include <vector>

class MarchingCubes {
 public:
  MarchingCubes(
      std::array<std::pair<std::bitset<5>, std::bitset<8>>, 6>& faces);

  std::pair<std::bitset<5>, std::bitset<8>> front() const;
  std::pair<std::bitset<5>, std::bitset<8>> back() const;
  std::pair<std::bitset<5>, std::bitset<8>> top() const;
  std::pair<std::bitset<5>, std::bitset<8>> bottom() const;
  std::pair<std::bitset<5>, std::bitset<8>> left() const;
  std::pair<std::bitset<5>, std::bitset<8>> right() const;

  // Calculate all inner cube edges, determine if cube center node is generated
  std::pair<std::vector<std::pair<uint8_t, uint8_t>>, bool> run();

 private:
  const std::array<std::pair<std::bitset<5>, std::bitset<8>>, 6> faces;

  /*
   * Edges are stored in the format (x, y) with the following vertex naming
   * scheme:
   *
   *  n: <face> <vertex position on face>
   *
   *  0: FRONT TOP
   *  1: FRONT LEFT
   *  2: FRONT RIGHT
   *  3: FRONT BOTTOM
   *  4: FRONT CENTER
   *  5: BACK TOP
   *  6: BACK LEFT
   *  7: BACK RIGHT
   *  8: BACK BOTTOM
   *  9: BACK CENTER
   * 11: TOP LEFT
   * 12: TOP RIGHT
   * 14: TOP CENTER
   * 16: BOTTOM LEFT
   * 17: BOTTOM RIGHT
   * 19: BOTTOM CENTER
   * 24: LEFT CENTER
   * 29: RIGHT CENTER
   * 42: CUBE CENTER
   *
   * TODO: Consider use of data structure without possible duplicates
   */
  std::vector<std::pair<uint8_t, uint8_t>> innerEdges;
  bool cubeCenterNode = false;

  uint8_t faceCenterNodeCount;

  uint8_t countFaceCenterNodes();
  void triangulate(std::vector<uint8_t>);
};

#endif  // MARCHINGCUBES_H
