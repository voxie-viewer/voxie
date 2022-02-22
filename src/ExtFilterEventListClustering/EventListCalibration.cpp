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

#include "EventListCalibration.hpp"

#include <VoxieClient/Exception.hpp>

#include <QDebug>
#include <QIODevice>
#include <QtXml/QDomDocument>

#include <cmath>
#include <cstring>

namespace vx {
namespace t3r {

void EventListCalibration::loadCalibrationFromXML(QIODevice* xmlInput) {
  QDomDocument document;
  document.setContent(xmlInput);

  auto parseCalibration = [](CalibrationArray& array, const QDomNode& node) {
    if (node.firstChild().isText()) {
      int arrayByteSize = array.size() * sizeof(CalibrationArray::value_type);
      QByteArray bytes =
          QByteArray::fromBase64(node.firstChild().toText().data().toUtf8());
      if (bytes.size() == arrayByteSize) {
        std::memcpy(array.data(), bytes.data(), bytes.size());
      } else {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtFilterEventListClustering",
            "Malformed calibration data (list size mismatch)");
      }
    } else {
      throw vx::Exception("de.uni_stuttgart.Voxie.ExtFilterEventListClustering",
                          "Missing calibration data within XML");
    }
  };

  auto children = document.lastChild().childNodes();
  for (int i = 0; i < children.size(); ++i) {
    auto child = children.at(i);

    // Search for the node named after the chipset ID (always contains a -)
    if (child.nodeName().contains("-")) {
      parseCalibration(calibrationA, child.namedItem("caliba"));
      parseCalibration(calibrationB, child.namedItem("calibb"));
      parseCalibration(calibrationC, child.namedItem("calibc"));
      parseCalibration(calibrationT, child.namedItem("calibt"));
      return;
    }
  }

  throw vx::Exception(
      "de.uni_stuttgart.Voxie.ExtFilterEventListClustering",
      "Malformed calibration data (could not locate chipset ID)");
}

void EventListCalibration::precomputeEnergyLookupTable(
    ShortTimestamp maximumToT) {
  energyLookupTableSize = maximumToT;
  energyLookupTable.resize(energyLookupTableSize * MatrixPixelCount);

  for (unsigned int y = 0; y < MatrixRowCount; ++y) {
    for (unsigned int x = 0; x < MatrixColumnCount; ++x) {
      for (unsigned int tot = 0; tot < maximumToT; ++tot) {
        energyLookupTable[getEnergyLookupTableIndex(x, y, tot)] =
            calculateEnergy(x, y, tot);
      }
    }
  }
}

Energy EventListCalibration::getEnergy(Coord x, Coord y,
                                       ShortTimestamp timeOverThreshold) const {
  return timeOverThreshold < energyLookupTableSize
             ? energyLookupTable[getEnergyLookupTableIndex(x, y,
                                                           timeOverThreshold)]
             : calculateEnergy(x, y, timeOverThreshold);
}

std::array<double, 4> EventListCalibration::getCoefficients(Coord x,
                                                            Coord y) const {
  auto index = x + y * 256;
  return std::array<double, 4>({calibrationA[index], calibrationB[index],
                                calibrationC[index], calibrationT[index]});
}

const EventListCalibration& EventListCalibration::getDefaultCalibration() {
  static auto fillArray = [](double value) {
    CalibrationArray array;
    array.fill(value);
    return array;
  };

  static EventListCalibration defaultCalibration(
      std::array<CalibrationArray, 4>(
          {{fillArray(0.5), fillArray(0), fillArray(0), fillArray(0)}}));

  return defaultCalibration;
}

EventListCalibration::EventListCalibration(
    const std::array<CalibrationArray, 4>& arrays)
    : calibrationA(arrays[0]),
      calibrationB(arrays[1]),
      calibrationC(arrays[2]),
      calibrationT(arrays[3]) {}

Energy EventListCalibration::calculateEnergy(
    Coord x, Coord y, ShortTimestamp timeOverThreshold) const {
  std::size_t index = x + y * MatrixColumnCount;

  double a = calibrationA[index];
  double b = calibrationB[index];
  double c = calibrationC[index];
  double t = calibrationT[index];

  // Use quadratic formula to determine energy from time over threshold
  double A = 2 * a;
  double B = b - a * t - timeOverThreshold;
  double C = t * timeOverThreshold - b * t - c;
  double D = B * B - 4 * a * C;

  return D >= 0 && !qFuzzyIsNull(A) ? (std::sqrt(D) - B) / A : 0;
}

std::size_t EventListCalibration::getEnergyLookupTableIndex(
    Coord x, Coord y, ShortTimestamp timeOverThreshold) const {
  return x + y * MatrixColumnCount + timeOverThreshold * MatrixPixelCount;
}

}  // namespace t3r
}  // namespace vx
