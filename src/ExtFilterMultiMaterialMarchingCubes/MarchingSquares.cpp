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

#include "MarchingSquares.hpp"

MarchingSquares::MarchingSquares(std::array<uint8_t, 4>& corners)
    : corners(std::ref(corners)) {}

uint8_t MarchingSquares::upperLeft() const { return corners[0]; }

uint8_t MarchingSquares::upperRight() const { return corners[1]; }

uint8_t MarchingSquares::lowerLeft() const { return corners[2]; }

uint8_t MarchingSquares::lowerRight() const { return corners[3]; }

std::pair<std::bitset<5>, std::bitset<8>> MarchingSquares::run() {
  generateVertices();
  generateEdges();

  return std::pair<std::bitset<5>, std::bitset<8>>(vertices, edges);
}

/*
 * Generate vertices on rectangle sides where materials on corresponding corners
 * do not match Does NOT generate centre nodes
 */
void MarchingSquares::generateVertices() {
  if (upperLeft() != upperRight()) vertices.set(0, true);
  if (upperLeft() != lowerLeft()) vertices.set(1, true);
  if (upperRight() != lowerRight()) vertices.set(2, true);
  if (lowerLeft() != lowerRight()) vertices.set(3, true);
}

/*
 * Generate edges and cube center nodes (when required).
 * generateVetices() MUST be executed first
 */
void MarchingSquares::generateEdges() {
  uint8_t vertexCount = vertices.count();

  // Case 0: 1 material
  if (vertexCount == 0) {
    // do nothing
  }
  // Case 1: only 2 materials
  else if (vertexCount == 2) {
    if (vertices[0]) {
      if (vertices[1]) {
        /*
         *  +   x   +
         *    /
         *  x
         *
         *  +       +
         */
        edges.set(0, true);
      } else if (vertices[2]) {
        /*
         *  +   x   +
         *        \
         *  x       x
         *
         *  +       +
         */
        edges.set(1, true);
      } else if (vertices[3]) {
        /*
         *  +   x   +
         *      |
         *
         *      |
         *  +   x   +
         */
        edges.set(4, true);
        edges.set(7, true);
      }
    } else if (vertices[1]) {
      if (vertices[2]) {
        /*
         *  +       +
         *
         *  x -   - x
         *
         *  +       +
         */
        edges.set(1, true);
      } else if (vertices[3]) {
        /*
         *  +       +
         *
         *  x
         *    \
         *  +   x   +
         */
        edges.set(4, true);
        edges.set(7, true);
      }
    } else if (vertices[2] && vertices[3]) {
      /*
       *  +       +
       *
       *          x
       *        /
       *  +   x   +
       */
      edges.set(3, true);
    }
  }
  // Case 2: same materials on adjacent corners (3 materials)
  else if (vertexCount == 3) {
    // Generate center node
    vertices.set(4, true);

    if (!vertices[0]) {
      /*
       *  +       +
       *
       *  x - x - x
       *      |
       *  +   x   +
       */
      edges.set(5, true);
      edges.set(6, true);
      edges.set(7, true);
    }
    if (!vertices[1]) {
      /*
       *  +   x   +
       *      |
       *      x - x
       *      |
       *  +   x   +
       */
      edges.set(4, true);
      edges.set(6, true);
      edges.set(7, true);
    }
    if (!vertices[2]) {
      /*
       *  +   x   +
       *      |
       *  x - x
       *      |
       *  +   x   +
       */
      edges.set(4, true);
      edges.set(5, true);
      edges.set(7, true);
    }
    if (!vertices[3]) {
      /*
       *  +   x   +
       *      |
       *  x - x - x
       *
       *  +       +
       */
      edges.set(4, true);
      edges.set(5, true);
      edges.set(6, true);
    }
  }
  // Case 3: same materials on opposing corners (3 materials)
  else if ((upperLeft() == lowerRight()) && !(upperRight() == lowerLeft())) {
    /*
     *  +   x   +
     *        \
     *  x       x
     *    \
     *  +   x   +
     */
    edges.set(1, true);
    edges.set(2, true);
  } else if ((upperRight() == lowerLeft()) && !(upperLeft() == lowerRight())) {
    /*
     *  +   x   +
     *    /
     *  x       x
     *        /
     *  +   x   +
     */
    edges.set(0, true);
    edges.set(3, true);
  }
  // Case 4: 4 materials (or ambiguous Case 3)
  else {
    /*
     *  0   x   1
     *      |
     *  x - x - x
     *      |
     *  2   x   3
     *
     * or
     *
     *  0   x   1
     *    ? | ?
     *  x - x - x
     *    ? | ?
     *  1   x   0
     *
     * Ambiguous Case 3 is handled like Case 4 in this implementation
     */
    vertices.set(4, true);
    edges.set(4, true);
    edges.set(5, true);
    edges.set(6, true);
    edges.set(7, true);
  }
}
