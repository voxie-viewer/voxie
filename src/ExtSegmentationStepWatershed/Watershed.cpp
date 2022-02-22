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

#include "Watershed.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <random>

Watershed::Watershed(const vx::Array3<const float>& volumeData,
                     const vx::Array3<vx::SegmentationType>& labelData,
                     const QList<vx::SegmentationType>& labelList, int sigma)
    : volumeData(volumeData),
      labelData(labelData),
      labelList(labelList),
      sigma(sigma) {
  this->nx = labelData.size<0>();
  this->ny = labelData.size<1>();
  this->nz = labelData.size<2>();
  for (auto labelID : labelList) {
    this->labelVoxelCounts.insert(labelID, 0);
  }

  // The blur weights can be precomputed since they will not change
  for (int i = -1; i <= 1; i++) {
    this->blurWeights[i + 1] =
        (1.0 / std::sqrt(2.0 * M_PI * std::pow(sigma, 2))) *
        std::exp(-std::pow(i, 2) / (2.0 * std::pow(sigma, 2)));
  }
}

void Watershed::run(
    vx::ClaimedOperation<
        de::uni_stuttgart::Voxie::ExternalOperationRunSegmentationStep>& op) {
  auto cmp = [](WatershedVoxel left, WatershedVoxel right) {
    return left.value > right.value;
  };
  std::priority_queue<WatershedVoxel, std::vector<WatershedVoxel>,
                      decltype(cmp)>
      q(cmp);

  // Set all other labels to 0 or clear highest bit
  for (size_t x = 0; x < nx; x++) {
    for (size_t y = 0; y < ny; y++) {
      for (size_t z = 0; z < nz; z++) {
        if (!labelList.contains(labelData(x, y, z) & 0x7f)) {
          labelData(x, y, z) = 0;
        } else {
          labelData(x, y, z) &= 0x7f;
        }
      }
    }
  }

  // Put all neighbours of labels into the queue
  for (size_t x = 0; x < nx; x++) {
    for (size_t y = 0; y < ny; y++) {
      for (size_t z = 0; z < nz; z++) {
        // Ignore marked and unlabeled voxels
        if (labelData(x, y, z) == 0 || labelData(x, y, z) == 0x80) {
          continue;
        }

        labelVoxelCounts[labelData(x, y, z)]++;

        // The highest bit is used to mark voxels we already put into the queue
        applyActionOnNeighbours(
            x, y, z, [this, &q](size_t i, size_t j, size_t k) {
              // Only add and mark neighbours that are
              // unmarked and unlabeled
              if (labelData(i, j, k) == 0) {
                float value = calculateGradientOfBlur(i, j, k);

                q.push(WatershedVoxel(value, i, j, k));
                labelData(i, j, k) = 0x80;
              }
            });
      }
    }
  }

  qDebug() << "Queue size: " << q.size();

  // Go through each item in the queue
  uint64_t counter = 0;
  uint64_t voxelCount = nx * ny * nz;
  while (!q.empty()) {
    WatershedVoxel c = q.top();
    q.pop();

    // Get neighbour labels
    std::array<vx::SegmentationType, 26> labels;
    int changes = 0;
    applyActionOnNeighbours(
        c.x, c.y, c.z, [this, &labels, &changes](size_t x, size_t y, size_t z) {
          if (labelData(x, y, z) != 0 && (labelData(x, y, z) & 0x80) == 0) {
            labels[changes] = labelData(x, y, z);
            changes++;
          }
        });

    Q_ASSERT(changes > 0);

    vx::SegmentationType label;
    if (changes > 1) {
#if true
      label = labels[0];
#else
      // Choose the most frequent label if neighbours have multiple labels
      // This has either bugs or it just is not a good selection strategy
      std::sort(labels.begin(), labels.end());
      int highestAmount = 1;
      int amount = 1;
      label = labels[0];

      bool first = false;
      bool lastLabel = label;
      for (vx::SegmentationType l : labels) {
        if (first) {
          first = false;
          continue;
        }
        if (label == l) {
          highestAmount++;
        } else {
          if (l != lastLabel) {
            lastLabel = label;
            amount = 1;
          } else {
            amount++;
          }

          if (amount > highestAmount) {
            label = l;
            highestAmount = amount;
          }
        }
      }
#endif
    } else {
      label = labels[0];
    }

    labelData(c.x, c.y, c.z) = label;
    labelVoxelCounts[label]++;

    // Add unlabeled and unmarked neighbours to queue
    applyActionOnNeighbours(c.x, c.y, c.z,
                            [this, &q](size_t x, size_t y, size_t z) {
                              if (labelData(x, y, z) == 0) {
                                float value = calculateGradientOfBlur(x, y, z);

                                q.push(WatershedVoxel(value, x, y, z));
                                labelData(x, y, z) = 0x80;
                              }
                            });

    counter++;
    // Progressbar (only update every percent)
    if (counter % (voxelCount / 100) == 0) {
      qDebug() << "Watershed progress: "
               << (int)(static_cast<double>(counter) /
                        static_cast<double>(voxelCount) * 100)
               << "%, Queue size: " << q.size();
      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
          static_cast<double>(counter) / static_cast<double>(voxelCount),
          vx::emptyOptions()));
    }
  }

  // Clear the highest bit
  for (size_t x = 0; x < nx; x++) {
    for (size_t y = 0; y < ny; y++) {
      for (size_t z = 0; z < nz; z++) {
        labelData(x, y, z) &= 0x7f;
      }
    }
  }

  HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(1.00, vx::emptyOptions()));
}

void Watershed::applyActionOnNeighbours(
    size_t x, size_t y, size_t z,
    std::function<void(size_t, size_t, size_t)> predicate) const {
  if (x != 0) {
    predicate(x - 1, y, z);
  }
  if (x < nx - 1) {
    predicate(x + 1, y, z);
  }
  if (y != 0) {
    predicate(x, y - 1, z);
  }
  if (y < ny - 1) {
    predicate(x, y + 1, z);
  }
  if (z != 0) {
    predicate(x, y, z - 1);
  }
  if (z < nz - 1) {
    predicate(x, y, z + 1);
  }
}

float Watershed::calculateBlur(size_t x, size_t y, size_t z) const {
  // Apply a gaussian 1D blur for each dimension
  float value = 0;
  float weightSum = 0;
  for (size_t i = x == 0 ? 0 : x - 1; i <= (x >= nx - 1 ? nx - 1 : x + 1);
       i++) {
    float weight = blurWeights[i - x + 1];
    weightSum += weight;
    value += weight * volumeData(i, y, z);
  }

  float oldValue = value;
  value = 0;
  for (size_t j = y == 0 ? 0 : y - 1; j <= (y >= ny - 1 ? ny - 1 : y + 1);
       j++) {
    float weight = blurWeights[j - y + 1];
    weightSum += weight;
    if (j == y) {
      value += weight * oldValue;
    } else {
      value += weight * volumeData(x, j, z);
    }
  }

  oldValue = value;
  value = 0;
  for (size_t k = z == 0 ? 0 : z - 1; k <= (z >= nz - 1 ? nz - 1 : z + 1);
       k++) {
    float weight = blurWeights[k - z + 1];
    weightSum += weight;
    if (k == z) {
      value += weight * oldValue;
    } else {
      value += weight * volumeData(x, y, k);
    }
  }

  return value / weightSum;
}

float Watershed::calculateGradientOfBlur(size_t x, size_t y, size_t z) const {
  float value = 0;
  if (x > 0 && x < nx - 1) {
    value +=
        std::pow(calculateBlur(x - 1, y, z) - calculateBlur(x + 1, y, z), 2);
  }
  if (y > 0 && y < ny - 1) {
    value +=
        std::pow(calculateBlur(x, y - 1, z) - calculateBlur(x, y + 1, z), 2);
  }
  if (z > 0 && z < nz - 1) {
    value +=
        std::pow(calculateBlur(x, y, z - 1) - calculateBlur(x, y, z + 1), 2);
  }
  return value;
}
