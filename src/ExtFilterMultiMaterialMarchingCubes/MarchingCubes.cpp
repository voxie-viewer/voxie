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

#include "MarchingCubes.hpp"

MarchingCubes::MarchingCubes(
    std::array<std::pair<std::bitset<5>, std::bitset<8>>, 6>& faces)
    : faces(std::ref(faces)) {
  countFaceCenterNodes();
}

std::pair<std::vector<std::pair<uint8_t, uint8_t>>, bool> MarchingCubes::run() {
  // TODO: pay attention to doubled nodes
  // Case 1: 0 face center nodes
  if (faceCenterNodeCount == 0) {
    // TODO: just triangulate
  }
  // Case 2: 2 face center nodes
  else if (faceCenterNodeCount == 2) {
    std::pair<uint8_t, uint8_t> centerConnectorEdge(255, 255);
    for (uint8_t i = 0; i < 6; i++) {
      // prevent double scanning, it is already known, there are exactly 2
      if (centerConnectorEdge.first == 255) {
        if (faces[i].first[4]) centerConnectorEdge.first = (5 * i) - 1;
      } else {
        if (faces[i].first[4]) centerConnectorEdge.second = (5 * i) - 1;
      }
    }

    // TODO: triangulate

    innerEdges.push_back(centerConnectorEdge);
  }
  // Case 3: 3 or more face center nodes
  else {
    // create cube center node and connect all existing nodes to it
    cubeCenterNode = true;
    for (uint8_t i = 0; i < 5; i++) {
      if (front().first[i]) innerEdges.push_back(std::make_pair(i, 42));
      if (back().first[i]) innerEdges.push_back(std::make_pair(i + 5, 42));
    }
    if (top().first[1]) innerEdges.push_back(std::make_pair(11, 42));
    if (top().first[2]) innerEdges.push_back(std::make_pair(12, 42));
    if (top().first[4]) innerEdges.push_back(std::make_pair(14, 42));
    if (bottom().first[1]) innerEdges.push_back(std::make_pair(16, 42));
    if (bottom().first[2]) innerEdges.push_back(std::make_pair(17, 42));
    if (bottom().first[4]) innerEdges.push_back(std::make_pair(19, 42));
    if (left().first[4]) innerEdges.push_back(std::make_pair(24, 42));
    if (right().first[4]) innerEdges.push_back(std::make_pair(29, 42));
  }

  return std::pair<std::vector<std::pair<uint8_t, uint8_t>>, bool>(
      innerEdges, cubeCenterNode);
}

std::pair<std::bitset<5>, std::bitset<8>> MarchingCubes::front() const {
  return faces[0];
}

std::pair<std::bitset<5>, std::bitset<8>> MarchingCubes::back() const {
  return faces[1];
}

std::pair<std::bitset<5>, std::bitset<8>> MarchingCubes::top() const {
  return faces[2];
}

std::pair<std::bitset<5>, std::bitset<8>> MarchingCubes::bottom() const {
  return faces[3];
}

std::pair<std::bitset<5>, std::bitset<8>> MarchingCubes::left() const {
  return faces[4];
}

std::pair<std::bitset<5>, std::bitset<8>> MarchingCubes::right() const {
  return faces[5];
}

uint8_t MarchingCubes::countFaceCenterNodes() {
  uint8_t faceCenterCount = 0;
  for (auto& face : faces) {
    if (face.first[4]) faceCenterCount++;
  }
  return faceCenterCount;
}

void MarchingCubes::triangulate(std::vector<uint8_t>) {
  // TODO: implement triangulation for closed polygon of n verteces
  innerEdges.push_back(std::make_pair(255, 255));  // placeholder
}
