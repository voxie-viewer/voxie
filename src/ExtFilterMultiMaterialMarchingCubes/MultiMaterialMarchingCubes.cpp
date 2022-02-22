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

#include "MultiMaterialMarchingCubes.hpp"
#include "MarchingCubes.hpp"
#include "MarchingSquares.hpp"

#include <future>

MultiMaterialMarchingCubes::MultiMaterialMarchingCubes(
    vx::Array3<const uint8_t> inputVolume, vx::Array3<uint8_t> outputVolume)
    : voxels(std::ref(inputVolume)),
      batchCountMinor(16)  // TODO: not use arbitrary number
{
  (void)outputVolume;  // TODO
}

void MultiMaterialMarchingCubes::run() {
  /*
   * Idea:
   * Scan for 3 layers of the volume (or 2 layers of cubes) at a time with
   * marching squares and store calculated nodes. Run Marching cubes on 2 cube
   * layers (except for first or last layer). Then delete temporary data of both
   * layers to free memory.
   *
   * Some partial functions are already implemented and should work.
   * What currently is missing are potentially those partial funktions for the
   * cubes step, triangulation in cubes step and
   */

  if (voxels.size<0>() < 2 || voxels.size<1>() < 4 || voxels.size<2>() < 2)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterMultiMaterialMarchingCubes.Error",
        "Volume is not tall enough (< 2x4x2)");

  // TODO: add main loop that traverses volume in batches of batchSize layers
}

vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*
MultiMaterialMarchingCubes::marchingSquaresLayerXZ(size_t y) {
  auto* layer = new vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>(
      {voxels.size<0>(), voxels.size<2>()});
  for (size_t x = 0; x < voxels.size<0>() - 1; x++) {
    for (size_t z = 0; z < voxels.size<2>() - 1; z++) {
      std::array<uint8_t, 4> corners;
      corners[0] = voxels(x, y, z);
      corners[1] = voxels(x + 1, y, z);
      corners[2] = voxels(x, y, z + 1);
      corners[3] = voxels(x + 1, y, z + 1);
      MarchingSquares square(corners);
      (*layer)(x, z) = square.run();
    }
  }
  return layer;
}

vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*
MultiMaterialMarchingCubes::marchingSquaresLayerYZ(size_t x, size_t yBegin,
                                                   size_t yEnd) {
  auto* layer = new vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>(
      {yEnd - yBegin, voxels.size<2>()});
  for (size_t y = yBegin; y < yEnd; y++) {
    for (size_t z = 0; z < voxels.size<2>() - 1; z++) {
      std::array<uint8_t, 4> corners;
      corners[0] = voxels(x, y, z);
      corners[1] = voxels(x, y + 1, z);
      corners[2] = voxels(x, y, z + 1);
      corners[3] = voxels(x, y + 1, z + 1);
      MarchingSquares square(corners);
      (*layer)(y, z) = square.run();
    }
  }
  return layer;
}

vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*
MultiMaterialMarchingCubes::marchingSquaresLayerXY(size_t z, size_t yBegin,
                                                   size_t yEnd) {
  auto* layer = new vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>(
      {voxels.size<0>(), yEnd - yBegin});
  for (size_t y = yBegin; y < yEnd; y++) {
    for (size_t x = 0; x < voxels.size<0>() - 1; x++) {
      std::array<uint8_t, 4> corners;
      corners[0] = voxels(x, y, z);
      corners[1] = voxels(x + 1, y, z);
      corners[2] = voxels(x, y + 1, z);
      corners[3] = voxels(x + 1, y + 1, z);
      MarchingSquares square(corners);
      (*layer)(x, y) = square.run();
    }
  }
  return layer;
}

std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
MultiMaterialMarchingCubes::marchingSquaresAxisY(size_t yBegin, size_t yEnd) {
  std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*> layers;
  layers.reserve(batchSize);

  std::vector<
      std::future<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>>
      layerFutures;
  layerFutures.reserve(batchSize);
  for (size_t i = yBegin; i <= yEnd; i++) {
    layerFutures.push_back(
        std::async([&]() { return marchingSquaresLayerXZ(i); }));
  }
  for (auto& future : layerFutures) {
    layers.push_back(future.get());
  }
  return layers;
}

std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
MultiMaterialMarchingCubes::marchingSquaresAxisZ(size_t yBegin, size_t yEnd) {
  std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*> layers;
  layers.reserve(batchSize);

  std::vector<std::future<
      std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>>>
      layerFutures;
  layerFutures.reserve(batchCountMinor);

  // split total layers over specified amount of threads
  for (size_t i = 0; i < batchCountMinor - 1; i++) {
    layerFutures.push_back(std::async(
        [&](size_t begin, size_t end) {
          std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
              batchLayers;
          batchLayers.reserve(voxels.size<2>() / batchCountMinor);
          for (size_t j = begin; j <= end; j++) {
            batchLayers.push_back(marchingSquaresLayerXY(j, yBegin, yEnd));
          }
          return batchLayers;
        },
        i * (voxels.size<2>() / batchCountMinor),
        (i + 1) * (voxels.size<2>() / batchCountMinor)));
  }
  // last batch is potentially larger than other fractions
  layerFutures.push_back(std::async(
      [&](size_t begin, size_t end) {
        std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
            batchLayers;
        batchLayers.reserve(voxels.size<2>() / batchCountMinor);
        for (size_t j = begin; j <= end; j++) {
          batchLayers.push_back(marchingSquaresLayerXY(j, yBegin, yEnd));
        }
        return batchLayers;
      },
      voxels.size<2>() - (voxels.size<2>() % batchCountMinor),
      voxels.size<2>()));  // TODO: check if layer is not processed twice

  // Collect all layers into one list
  for (auto& future : layerFutures) {
    for (auto& layer : future.get()) {
      layers.push_back(layer);
    }
  }
  return layers;
}

std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
MultiMaterialMarchingCubes::marchingSquaresAxisX(size_t yBegin, size_t yEnd) {
  std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*> layers;
  layers.reserve(batchSize);

  std::vector<std::future<
      std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>>>
      layerFutures;
  layerFutures.reserve(batchCountMinor);

  // split total layers over specified amount of threads
  for (size_t i = 0; i < batchCountMinor - 1; i++) {
    layerFutures.push_back(std::async(
        [&](size_t begin, size_t end) {
          std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
              batchLayers;
          batchLayers.reserve(voxels.size<2>() / batchCountMinor);
          for (size_t j = begin; j <= end; j++) {
            batchLayers.push_back(marchingSquaresLayerYZ(j, yBegin, yEnd));
          }
          return batchLayers;
        },
        i * (voxels.size<2>() / batchCountMinor),
        (i + 1) * (voxels.size<2>() / batchCountMinor)));
  }
  // last batch is potentially larger than other fractions
  layerFutures.push_back(std::async(
      [&](size_t begin, size_t end) {
        std::vector<vx::Array2<std::pair<std::bitset<5>, std::bitset<8>>>*>
            batchLayers;
        batchLayers.reserve(voxels.size<2>() / batchCountMinor);
        for (size_t j = begin; j <= end; j++) {
          batchLayers.push_back(marchingSquaresLayerYZ(j, yBegin, yEnd));
        }
        return batchLayers;
      },
      voxels.size<2>() - (voxels.size<2>() % batchCountMinor),
      voxels.size<2>()));  // TODO: check if layer is not processed twice

  // Collect all layers into one list
  for (auto& future : layerFutures) {
    for (auto& layer : future.get()) {
      layers.push_back(layer);
    }
  }
  return layers;
}
