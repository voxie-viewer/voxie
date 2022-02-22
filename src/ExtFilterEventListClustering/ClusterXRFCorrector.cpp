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

#include "ClusterXRFCorrector.hpp"

#include <algorithm>

namespace vx {
namespace t3r {

ClusterXRFCorrector::ClusterXRFCorrector(XRFCorrectionSettings settings)
    : settings(settings) {}

void ClusterXRFCorrector::performXRFCorrection(
    std::vector<EventClusterer::Cluster>& clusters, std::size_t minIndex,
    std::size_t maxIndex) {
  maxIndex = std::min(maxIndex, clusters.size());

  // Prerequisite: Cluster list is ordered by timestamp.
  for (size_t i = minIndex; i < maxIndex; ++i) {
    auto& curCluster = clusters[i];

    // Check if energy is between chosen threshold. If not, ignore.
    float energy = curCluster.energy;
    if (energy < settings.minEnergy || energy > settings.maxEnergy) {
      continue;
    }

    // Keep track of the closest cluster meeting the requirements.
    int64_t closestClusterIndex = -1;
    float closestSqDistance = settings.distanceLimit * settings.distanceLimit;

    // Will be used inside the loop to refresh the closest cluster.
    auto checkAndUpdateClosestCluster = [&](size_t index) {
      auto& checkCluster = clusters[index];

      // Check whether the timestamp is in range of the interval.
      if (std::abs(curCluster.timestamp - checkCluster.timestamp) >
          settings.temporalMargin) {
        return false;
      }

      // Check whether photon has already been merged (energy == 0)
      if (checkCluster.energy == 0.f) {
        return true;
      }

      // Check the distance of the cluster if applicable.
      float dx = curCluster.x - checkCluster.x;
      float dy = curCluster.y - checkCluster.y;

      float sqDistance = dx * dx + dy * dy;
      if (sqDistance < closestSqDistance) {
        closestSqDistance = sqDistance;
        closestClusterIndex = index;
      }

      return true;
    };

    // Check down first for closest fitting cluster until out of range or at 0.
    for (size_t j = i; j > minIndex; --j) {
      if (!checkAndUpdateClosestCluster(j - 1)) {
        break;
      }
    }

    // Check upwards for closest fitting cluster until out of range or at end.
    for (size_t j = i + 1; j < maxIndex; ++j) {
      if (!checkAndUpdateClosestCluster(j)) {
        break;
      }
    }

    // Check whether the found cluster satisfies conditions still.
    if (closestClusterIndex < 0) {
      continue;
    }

    auto& closestCluster = clusters[closestClusterIndex];

    // Merge clusters into the higher energetic one, set old one to 0.
    if (energy > closestCluster.energy) {
      curCluster.energy += closestCluster.energy;
      closestCluster.energy = 0.f;
    } else {
      closestCluster.energy += energy;
      curCluster.energy = 0.f;
    }
  }

  // Remove old clusters marked as "done".
  clusters.erase(
      std::remove_if(clusters.begin() + minIndex, clusters.begin() + maxIndex,
                     [](auto& cluster) { return cluster.energy == 0.f; }),
      clusters.begin() + maxIndex);
}

}  // namespace t3r
}  // namespace vx
