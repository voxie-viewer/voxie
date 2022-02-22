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

#ifndef BILATERALFILTERING_H
#define BILATERALFILTERING_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>

#include <cmath>

#include <QObject>

class BilateralFiltering {
 public:
  BilateralFiltering(double sigmaS, double sigmaP);
  void compute(vx::Array2<float>& vertices,
               vx::Array2<const uint32_t> triangles,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);

 private:
  QList<QVector3D> mollifiedNormals;
  QList<float> trianglesAreas;
  double sigmaSpacial;
  double sigmaPrediction;
  QList<QSet<int>> neighbourNodes;
  QList<QSet<int>> neighbourTriangles;
  QList<QVector3D> centroids;
  double meanEdgeLength;  // not correct for surfaces that are not closed,
                          // though still a reasonable estimation

  void setUpData(vx::Array2<float> vertices,
                 vx::Array2<const uint32_t> triangles);
  void mollifyNormals(
      vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
      vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void getNeighbourhood(int node, QSet<int>& neighbourhood, int circleSize);
  void getTriangleInfo(vx::Array2<float> vertices,
                       vx::Array2<const uint32_t> triangles);
  void evaluateNewPositions(
      vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles,
      double sigmaSp, double SigmaPred,
      vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void evaluatePositionDiff(QVector3D oldPos, QVector3D estimation, double area,
                            QVector3D centroid, double sigmaSp,
                            double SigmaPred, QVector3D& newPos,
                            double& weight);
  double evaluateGaussian(QVector3D point, QVector3D diff, double sigma);
  void calculateNormals(vx::Array2<float> vertices,
                        vx::Array2<const uint32_t> triangles,
                        QList<QVector3D> differences);
};

#endif  // BILATERALFILTERING_H
