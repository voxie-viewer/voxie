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

#ifndef HAUSDORFFDISTANCE_H
#define HAUSDORFFDISTANCE_H

#include <QObject>
#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusTypeList.hpp>
#include <array>
#include <cmath>
#include <iostream>
#include "Cell.hpp"

class HausdorffDistance {
 public:
  HausdorffDistance(vx::Array2<const uint32_t> triangles_nominal,
                    vx::Array2<const float> vertices_nominal,
                    vx::Array2<const uint32_t> triangles_actual,
                    vx::Array2<const float> vertices_actual,
                    vx::Array2<float> outputDistances);
  void run(vx::ClaimedOperation<
           de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& op);

 private:
  vx::Array2<const uint32_t> triangles_nominal;
  vx::Array2<const float> vertices_nominal;
  vx::Array2<const uint32_t> triangles_actual;
  vx::Array2<const float> vertices_actual;
  vx::Array2<float> outputDistances;

  double minimalDistance(const QVector3D point,
                         const vx::Array3<Cell> cellArray);
  double minimalDistanceInsideCell(QVector3D point, Cell cell);
  double calculateDistance(QVector3D point1, QVector3D point2);
  QVector3D calculateCell(QVector3D point, vx::Array3<Cell> cellArray);
  std::vector<std::vector<QVector3D>> getSamplingPoints(
      vx::Array2<const float>& actualVertices, const double samplingDensity);
  std::vector<QVector3D> triangleSampling(const QVector3D vertexA,
                                          const QVector3D vertexB,
                                          const QVector3D vertexC,
                                          int32_t sampleFrequency);
  int calculateSampleFrequency(const double triangleArea,
                               const double samplingDensity);
  double calculateTriangleArea(const QVector3D vertexA, const QVector3D vertexB,
                               const QVector3D vertexC);
};

#endif  // HAUSDORFFDISTANCE_H
