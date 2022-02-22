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

#ifndef MARCHINGSQUARES_H
#define MARCHINGSQUARES_H

#include <array>
#include <bitset>
#include <future>

class MarchingSquares {
 public:
  MarchingSquares(std::array<uint8_t, 4>& corners);

  uint8_t upperLeft() const;
  uint8_t upperRight() const;
  uint8_t lowerLeft() const;
  uint8_t lowerRight() const;

  std::pair<std::bitset<5>, std::bitset<8>> run();

 private:
  const std::array<uint8_t, 4>& corners;

  /*
   * Set bits represent generated nodes:
   *
   * 0: TOP
   * 1: LEFT
   * 2: RIGHT
   * 3: BOTTOM
   * 4: CENTER
   *
   *  +   x   +     +   0   +
   *
   *  x   x   x     1   4   2
   *
   *  +   x   +     +   3   +
   *
   * + : corner
   * x : vertex
   */
  std::bitset<5> vertices;
  /*
   * Set bits represent generated edges:
   *
   * 0: TOP-LEFT
   * 1: TOP-RIGHT
   * 2: BOTTOM-LEFT
   * 3: BOTTOM-RIGHT
   * 4: CENTER-TOP
   * 5: CENTER-LEFT
   * 6: CENTER-RIGHT
   * 7: CENTER-BOTTOM
   *
   *  +   x   +     +   x   +
   *    / | \         0 4 1
   *  x - x - x     x 5 x 6 x
   *    \ | /         2 7 3
   *  +   x   +     +   x   +
   *
   * + : corner
   * x : vertex
   * - : edge
   */
  std::bitset<8> edges;

  void generateVertices();
  void generateEdges();

  uint8_t getVertexCount();
};

#endif  // MARCHINGSQUARES_H
