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

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "Watershed.hpp"

void Watershed::runParallel(
    vx::ClaimedOperation<
        de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>& op) {
  // TODO Divide work onto threads and handle region borders
  createRegionSpanningTrees(0, 0, 0, nx, ny, nz, op);

  // TODO Merge regions

  HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(1.00, vx::emptyOptions()));
}

void Watershed::createRegionSpanningTrees(
    size_t x1, size_t y1, size_t z1, size_t x2, size_t y2, size_t z2,
    vx::ClaimedOperation<
        de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>& op) {
  qDebug() << "Stage one";
  std::vector<WatershedTree> trees;
  std::vector<WatershedEdge> edges;
  trees.reserve(nx * ny * nz);
  edges.reserve(nx * ny * nz * 3);

  // Create initial single node trees and their edges to other trees
  for (size_t x = x1; x < x2; x++) {
    for (size_t y = y1; y < y2; y++) {
      for (size_t z = z1; z < z2; z++) {
        vx::SegmentationType labelID = 0;
        if (labelList.contains(labelData(x, y, z) & 0x7f)) {
          labelID = labelData(x, y, z) & 0x7f;
        }

        size_t currentIndex = calculateIndex(x, y, z);
        float currentValue = calculateBlur(x, y, z);

        WatershedTree tree(currentIndex, labelID);

        if (x < nx - 1) {
          edges.push_back(WatershedEdge(
              currentIndex, calculateIndex(x + 1, y, z),
              std::pow(currentValue - calculateBlur(x + 1, y, z), 2)));
        }
        if (y < ny - 1) {
          edges.push_back(WatershedEdge(
              currentIndex, calculateIndex(x, y + 1, z),
              std::pow(currentValue - calculateBlur(x, y + 1, z), 2)));
        }
        if (z < nz - 1) {
          edges.push_back(WatershedEdge(
              currentIndex, calculateIndex(x, y, z + 1),
              std::pow(currentValue - calculateBlur(x, y, z + 1), 2)));
        }

        Q_ASSERT(trees.size() == currentIndex);
        trees.push_back(tree);
      }
    }
    qDebug() << x << "/" << nx;
    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
        static_cast<double>(x) / static_cast<double>(2 * nx),
        vx::emptyOptions()));
  }

  qDebug() << "Edges: " << edges.capacity();
  // Index of smallest edge for each tree
  std::vector<size_t> smallestEdges(trees.size(),
                                    std::numeric_limits<size_t>::max());

  // Boruvka algorithm
  size_t mergedTreeCount = 1;
  size_t remainingTrees = trees.size();
  while (mergedTreeCount > 0) {
    mergedTreeCount = 0;

    qDebug() << "Searching smallest edges";
    for (size_t i = 0; i < edges.size(); i++) {
      size_t tree1 = findRoot(trees, edges[i].source);
      size_t tree2 = findRoot(trees, edges[i].destination);

      // Check if trees are already merged
      if (tree1 == tree2) {
        continue;
      }

      // Check if trees have different labels
      if (trees[tree1].labelID != 0 &&
          trees[tree1].labelID != trees[tree2].labelID) {
        continue;
      }

      // Update smallest edges
      if (smallestEdges[tree1] == std::numeric_limits<size_t>::max() ||
          edges[smallestEdges[tree1]].weight > edges[i].weight) {
        smallestEdges[tree1] = i;
      }
      if (smallestEdges[tree2] == std::numeric_limits<size_t>::max() ||
          edges[smallestEdges[tree2]].weight > edges[i].weight) {
        smallestEdges[tree2] = i;
      }
    }

    qDebug() << "Merging trees";
    for (size_t i = 0; i < trees.size(); i++) {
      if (smallestEdges[i] != std::numeric_limits<size_t>::max()) {
        size_t tree1 = findRoot(trees, edges[smallestEdges[i]].source);
        size_t tree2 = findRoot(trees, edges[smallestEdges[i]].destination);
        smallestEdges[i] = std::numeric_limits<size_t>::max();

        if (tree1 == tree2) {
          continue;
        }
        if (trees[tree1].labelID != 0 &&
            trees[tree1].labelID != trees[tree2].labelID) {
          continue;
        }

        unite(trees, tree1, tree2);
        mergedTreeCount++;
        remainingTrees--;
      }
    }
    qDebug() << "Trees merged:" << mergedTreeCount
             << "Remaining:" << remainingTrees;
    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
        0.5 + static_cast<double>(trees.size() - remainingTrees) /
                  static_cast<double>(2 * trees.size()),
        vx::emptyOptions()));
  }

  qDebug() << "Assigning labels";
  // For testing: write label data
  for (size_t i = 0; i < trees.size(); i++) {
    vx::SegmentationType labelID = trees[findRoot(trees, i)].labelID;

    if (labelID != 0) {
      auto c = calculateCoordinates(i);
      labelData(std::get<0>(c), std::get<1>(c), std::get<2>(c)) = labelID;
      labelVoxelCounts[labelID]++;
    }
  }
  for (auto labelID : labelList) {
    qDebug() << labelID << labelVoxelCounts[labelID];
  }
}

size_t Watershed::calculateIndex(size_t x, size_t y, size_t z) const {
  return x * ny * nz + y * nz + z;
}

std::tuple<size_t, size_t, size_t> Watershed::calculateCoordinates(
    size_t index) const {
  return std::make_tuple(index / (ny * nz), (index % (ny * nz)) / nz,
                         index % nz);
}

int Watershed::findRoot(std::vector<WatershedTree>& trees, size_t i) {
  if (trees[i].parent != i) {
    trees[i].parent = findRoot(trees, trees[i].parent);
  }
  return trees[i].parent;
}

void Watershed::unite(std::vector<WatershedTree>& trees, size_t tree1,
                      size_t tree2) {
  size_t root1 = findRoot(trees, tree1);
  size_t root2 = findRoot(trees, tree2);

  if (trees[root1].rank < trees[root2].rank) {
    trees[root1].parent = root2;
    if (trees[root2].labelID == 0) {
      trees[root2].labelID = trees[root1].labelID;
    }
  } else if (trees[root1].rank > trees[root2].rank) {
    trees[root2].parent = root1;
    if (trees[root1].labelID == 0) {
      trees[root1].labelID = trees[root2].labelID;
    }
  } else {
    trees[root2].parent = root1;
    trees[root1].rank++;
    if (trees[root1].labelID == 0) {
      trees[root1].labelID = trees[root2].labelID;
    }
  }
}
