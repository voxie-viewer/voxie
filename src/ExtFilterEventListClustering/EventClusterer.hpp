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

#include "EventBucketer.hpp"
#include "EventListCalibration.hpp"

#include <array>
#include <vector>

namespace vx {
namespace t3r {

class EventClusterer {
 public:
  struct DetectorData {
    DetectorData(QDir workingDirectory,
                 const QMap<QString, QDBusVariant>& data);

    QString chipboardID;
    EventListCalibration calibration;
    std::array<double, 2> origin = {{0, 0}};
    std::array<double, 2> pixelSize = {{0, 0}};
    double rotation = 0;

    template <typename T>
    void transformPoint(T& x, T& y) const {
      T tx = transform[0] * x + transform[1] * y + origin[0];
      T ty = transform[2] * x + transform[3] * y + origin[1];
      x = tx;
      y = ty;
    }

   private:
    std::array<double, 4> transform;
  };

  struct Cluster {
    Timestamp timestamp = 0;
    float x = 0;
    float y = 0;
    float energy = 0;

    bool friend operator<(const Cluster& c1, const Cluster& c2) {
      return c1.timestamp < c2.timestamp;
    }
  };

  EventClusterer(const DetectorData& detectorData, EventBucketer& bucketer);

  void clusterEvents(std::vector<EventClusterer::Cluster>& clusters,
                     Timestamp minTime, Timestamp maxTime,
                     Timestamp temporalMargin);

 private:
  Cluster processCluster(Coord initX, Coord initY, Timestamp minTime,
                         Timestamp maxTime);

  void transformCluster(Cluster& cluster) const;

  const DetectorData& detectorData;
  EventBucketer& bucketer;
};

}  // namespace t3r
}  // namespace vx
