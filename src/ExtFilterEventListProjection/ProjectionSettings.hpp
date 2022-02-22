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

#include "EventProjectionCommon.hpp"

#include <VoxieClient/Exception.hpp>

#include <QMap>
#include <QString>

#include <cstdint>
#include <limits>

namespace vx {
namespace t3r {

enum class ProjectionAttribute {
  Energy,
  EventCount,
  TimeOfArrival,
};

enum class ProjectionMode {
  Sum,
  Mean,
  Maximum,
  Minimum,
};

struct ProjectionSettings {
  ProjectionAttribute attribute = ProjectionAttribute::Energy;
  ProjectionMode mode = ProjectionMode::Sum;
  double minEnergy = 0;
  double maxEnergy = 10000;
  Timestamp minTime = std::numeric_limits<Timestamp>::min();
  Timestamp maxTime = std::numeric_limits<Timestamp>::max();
  std::int32_t imageWidth = 256;
  std::int32_t imageHeight = 256;
  bool enableWhiteImage = false;
};

struct ProjectionRegionSettings {
  std::uint32_t outputImageID = 0;
  std::uint32_t outputX = 0;
  std::uint32_t outputY = 0;
  std::uint32_t inputX = 0;
  std::uint32_t inputY = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

#define VX_DETAIL_ATTRIB(NAME)                                            \
  {                                                                       \
    "de.uni_stuttgart.Voxie.Filter.EventListProjection.Attribute." #NAME, \
        ProjectionAttribute::NAME                                         \
  }

#define VX_DETAIL_MODE(NAME)                                         \
  {                                                                  \
    "de.uni_stuttgart.Voxie.Filter.EventListProjection.Mode." #NAME, \
        ProjectionMode::NAME                                         \
  }

inline ProjectionAttribute projectionAttributeFromString(QString string) {
  static QMap<QString, ProjectionAttribute> map = {
      VX_DETAIL_ATTRIB(Energy), VX_DETAIL_ATTRIB(EventCount),
      VX_DETAIL_ATTRIB(TimeOfArrival)};

  if (map.contains(string)) {
    return map[string];
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
        QString("Invalid projection attribute '%1'").arg(string));
  }
}

inline ProjectionMode projectionModeFromString(QString string) {
  static QMap<QString, ProjectionMode> map = {
      VX_DETAIL_MODE(Sum), VX_DETAIL_MODE(Mean), VX_DETAIL_MODE(Maximum),
      VX_DETAIL_MODE(Minimum)};

  if (map.contains(string)) {
    return map[string];
  } else {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.ExtFilterEventListProjection.Error",
        QString("Invalid projection mode '%1'").arg(string));
  }
}

#undef VX_DETAIL_MODE
#undef VX_DETAIL_ATTRIB

}  // namespace t3r
}  // namespace vx
