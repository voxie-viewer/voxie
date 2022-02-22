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

#include "EventClusterer.hpp"

#include "DBusNumericUtil.hpp"

#include <VoxieClient/DBusUtil.hpp>

#include <QDebug>

#include <unordered_set>
#include <utility>

namespace vx {
namespace t3r {

EventClusterer::DetectorData::DetectorData(
    QDir workingDirectory, const QMap<QString, QDBusVariant>& data) {
  // Helper function to convert a DBus variant to a 2D array
  using Vec2 = std::array<double, 2>;
  auto getDoubleArray = [](const QDBusVariant& variant) -> Vec2 {
    auto variantList = dbusGetVariantValue<QList<QDBusVariant>>(variant);
    return variantList.size() < 2
               ? Vec2{{0, 0}}
               : Vec2{{dbusGetNumber<double>(variantList[0]),
                       dbusGetNumber<double>(variantList[1])}};
  };

  // Load metadata
  chipboardID = dbusGetVariantValue<QString>(data.value("ChipboardID"));
  rotation = dbusGetNumber<double>(data.value("Rotation"));
  origin = getDoubleArray(data.value("Origin"));
  pixelSize = getDoubleArray(data.value("PixelSize"));

  // Load calibration matrix from XML file
  QFile calibrationFile(workingDirectory.filePath(
      dbusGetVariantValue<QString>(data.value("CalibrationFile"))));
  calibration.loadCalibrationFromXML(&calibrationFile);

  // Initialize transformation matrix
  double sinAngle = std::sin(rotation), cosAngle = std::cos(rotation);
  std::array<double, 4> rotation = {{cosAngle, sinAngle, -sinAngle, cosAngle}};
  std::array<double, 4> scale = {{pixelSize[0], 0, 0, pixelSize[1]}};

  transform = {{
      scale[0] * rotation[0] + scale[1] * rotation[2],
      scale[0] * rotation[1] + scale[1] * rotation[3],
      scale[2] * rotation[0] + scale[3] * rotation[2],
      scale[2] * rotation[1] + scale[3] * rotation[3],
  }};
}

EventClusterer::EventClusterer(const DetectorData& detectorData,
                               EventBucketer& bucketer)
    : detectorData(detectorData), bucketer(bucketer) {}

void EventClusterer::clusterEvents(
    std::vector<EventClusterer::Cluster>& clusters, Timestamp minTime,
    Timestamp maxTime, Timestamp temporalMargin) {
  // TODO increase search time margin for clusters to make edge cases consistent
  for (int y = 0; y < int(MatrixRowCount); ++y) {
    for (int x = 0; x < int(MatrixColumnCount); ++x) {
      Timestamp time;
      while (bucketer.findInitialTimestamp(x, y, time)) {
        auto cluster =
            processCluster(x, y, time - temporalMargin, time + temporalMargin);
        if (cluster.timestamp >= minTime && cluster.timestamp <= maxTime) {
          clusters.push_back(cluster);
        }
      }
    }
  }
}

EventClusterer::Cluster EventClusterer::processCluster(Coord initX, Coord initY,
                                                       Timestamp minTime,
                                                       Timestamp maxTime) {
  Cluster cluster;

  // Cluster timestamp initialized to maximum, as it is reduced within the loop
  cluster.timestamp = maxTime;
  float maxEnergy = -1.f;

  unsigned int eventCount = 0;

  std::vector<std::pair<Coord, Coord>> pendingPixels;
  std::array<bool, MatrixPixelCount> visitedPixels{};

  auto queue = [&](Coord x, Coord y) {
    auto index = x + y * MatrixRowCount;
    if (visitedPixels[index]) {
      return false;
    } else {
      pendingPixels.emplace_back(x, y);
      visitedPixels[index] = true;
      return true;
    }
  };

  queue(initX, initY);

  for (std::size_t i = 0; i < pendingPixels.size(); ++i) {
    Coord x = pendingPixels[i].first;
    Coord y = pendingPixels[i].second;

    // Acquire (and erase) all events in range on the current pixel
    bool found = false;

    if (auto range = bucketer.findRangeAt(x, y, minTime, maxTime)) {
      for (auto e = range.begin(); e != range.end(); ++e) {
        // Skip invalid events
        if (!e.isValid()) {
          continue;
        }

        // Check for max energy, if so, refresh timestamp
        float energy =
            detectorData.calibration.getEnergy(x, y, e.getTimeOverThreshold());
        if (energy > maxEnergy) {
          maxEnergy = energy;
          cluster.timestamp = e.getTimestamp();
        }

        // Add current event to cluster
        cluster.x += x;
        cluster.y += y;
        cluster.energy += energy;
        eventCount++;
        found = true;
      }

      // Mark range as "destroyed" to skip it in future iterations
      range.destroy();
    }

    // Check if any event was found within the current pixel
    if (found) {
      // Enqueue neighboring pixels for visitation
      for (int y2 = std::max(y - 1, 0);
           y2 <= std::min<int>(y + 1, MatrixColumnCount); ++y2) {
        for (int x2 = std::max(x - 1, 0);
             x2 <= std::min<int>(x + 1, MatrixRowCount); ++x2) {
          queue(x2, y2);
        }
      }
    }
  }

  // Compute average event position to obtain mean cluster position
  if (eventCount > 0) {
    cluster.x /= eventCount;
    cluster.y /= eventCount;
  }

  // Apply transformation based on detector data
  transformCluster(cluster);

  return cluster;
}

void EventClusterer::transformCluster(Cluster& cluster) const {
  // Center cluster on pixel
  cluster.x += 0.5f;
  cluster.y += 0.5f;
  detectorData.transformPoint(cluster.x, cluster.y);
}

}  // namespace t3r
}  // namespace vx
