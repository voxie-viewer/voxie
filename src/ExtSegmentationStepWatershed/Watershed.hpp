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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>

#include <Voxie/Node/Types.hpp>

class Watershed {
  struct WatershedVoxel {
    float value;
    size_t x;
    size_t y;
    size_t z;

    WatershedVoxel(float value, size_t x, size_t y, size_t z)
        : value(value), x(x), y(y), z(z){};
  };

  struct WatershedTree {
    size_t rank = 0;
    size_t parent = 0;
    bool isFinished = false;
    vx::SegmentationType labelID = 0;

    WatershedTree(){};
    WatershedTree(size_t parent, vx::SegmentationType labelID)
        : parent(parent), labelID(labelID){};
  };

  struct WatershedEdge {
    size_t source;
    size_t destination;
    size_t weight;

    WatershedEdge(){};
    WatershedEdge(size_t source, size_t destination, size_t weight)
        : source(source), destination(destination), weight(weight){};
  };

 public:
  Watershed(const vx::Array3<const float>& volumeData,
            const vx::Array3<vx::SegmentationType>& labelData,
            const QList<vx::SegmentationType>& labelList, int sigma);

  /**
   * @brief Run the sequential implementation
   */
  void run(vx::ClaimedOperation<
           de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>& op);

  /**
   * @brief Run the parallel implementation
   */
  void runParallel(
      vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>& op);

  QHash<vx::SegmentationType, size_t> labelVoxelCounts;

  size_t getVoxelCount() { return nx * ny * nz; };

 private:
  const vx::Array3<const float>& volumeData;
  const vx::Array3<vx::SegmentationType>& labelData;
  const QList<vx::SegmentationType>& labelList;
  int sigma;
  std::array<float, 3> blurWeights;

  size_t nx;
  size_t ny;
  size_t nz;

  /**
   * @brief Apply the predicate on all 6 neighbors of a voxel
   */
  void applyActionOnNeighbours(
      size_t x, size_t y, size_t z,
      std::function<void(size_t, size_t, size_t)> predicate) const;

  /**
   * @brief Calculate the gaussian blur at this position
   */
  float calculateBlur(size_t x, size_t y, size_t z) const;

  /**
   * @brief Calculate the gradient magnitude of the blurred neighbor values
   */
  float calculateGradientOfBlur(size_t x, size_t y, size_t z) const;

  /**
   * @brief Create a minimal spanning tree in this region
   */
  void createRegionSpanningTrees(
      size_t x1, size_t y1, size_t z1, size_t x2, size_t y2, size_t z2,
      vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>& op);

  /**
   * @brief Find root and make root as parent of i
   */
  static int findRoot(std::vector<WatershedTree>& trees, size_t i);

  /**
   * @brief Attach smaller rank tree under root of high rank tree
   */
  static void unite(std::vector<WatershedTree>& trees, size_t tree1,
                    size_t tree2);

  /**
   * @brief Calculate a index out of x, y and z coordinates
   */
  size_t calculateIndex(size_t x, size_t y, size_t z) const;

  /**
   * @brief Recalculate the coordinates from an index
   */
  std::tuple<size_t, size_t, size_t> calculateCoordinates(size_t index) const;
};
