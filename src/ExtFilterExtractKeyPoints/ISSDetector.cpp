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

#include "ISSDetector.hpp"
#include "Eigen3D.hpp"

#include <QDebug>
#include <cmath>

ISSDetector::ISSDetector() {}

std::vector<QVector3D> ISSDetector::detectKeypoints(KdTree<QVector3D>& input) {
  std::vector<QVector3D> output;
  const std::vector<QVector3D>& surface = input.getPcRef();
  std::vector<QVector3D> candidates;
  std::vector<float> eigenvalues3;
  float** cov = new float*[3];
  for (int i = 0; i < 3; i++) {
    cov[i] = new float[3];
  }
  float eigenValuesReal[3];
  float eigenValuesImag[3];
  float searchRadius = searchRadius_;
  float nonMaxRadius = nonMaxRadius_;

  if (searchRadius == 0 || nonMaxRadius == 0) {
    double resolution = computeResolution(input);
    if (searchRadius == 0) {
      searchRadius = 6 * resolution;
      qDebug() << "searchRadius: " << searchRadius;
    }
    if (nonMaxRadius == 0) {
      nonMaxRadius = 4 * resolution;
      qDebug() << "nonMaxRadius: " << nonMaxRadius;
    }
  }

  for (uint32_t index = 0; index < surface.size(); index++) {
    QVector3D currentPoint = surface[index];
    std::vector<QVector3D> nn = input.radiusSearch(currentPoint, searchRadius);

    if (nn.size() >= minNeighbors_) {
      // compute the scatter matrix
      getScatterMatrix(currentPoint, nn, cov);
      Eigen3D::eigenValues3(cov, eigenValuesReal, eigenValuesImag);
      double e1 = eigenValuesReal[0];
      double e2 = eigenValuesReal[1];
      double e3 = eigenValuesReal[2];

      // check condition for feature points
      if (std::isfinite(e1) && std::isfinite(e2) && std::isfinite(e3)) {
        if (e2 / e1 < gamma21_ && e3 / e2 < gamma32_ && e3 > 0) {
          candidates.push_back(currentPoint);
          eigenvalues3.push_back(e3);
        }
      }
    }
  }

  if (candidates.size() == 0) {
    return output;
  }
  KdTree<QVector3D> candidateTree(candidates);
  for (uint32_t i = 0; i < candidates.size(); i++) {
    QVector3D& currentPoint = candidates[i];
    double maxFeature = true;
    std::vector<uint32_t> nn =
        candidateTree.radiusSearchIndex(currentPoint, nonMaxRadius);
    for (uint32_t nn_index = 0; nn_index < nn.size(); nn_index++) {
      if (i == nn[nn_index]) {
        continue;
      }
      if (eigenvalues3[nn[nn_index]] >= eigenvalues3[i]) {
        maxFeature = false;
        break;
      }
    }
    if (maxFeature) {
      output.push_back(currentPoint);
    }
  }

  for (int i = 0; i < 3; i++) {
    delete[] cov[i];
  }
  delete[] cov;

  return output;
}

void ISSDetector::getScatterMatrix(QVector3D& currentPoint,
                                   std::vector<QVector3D>& nn, float** cov) {
  // use mean instead of currentPoint?
  //  QVector3D mean(0,0,0);
  //  for (QVector3D& neighbor : nn) {
  //    mean += neighbor/nn.size();
  //  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      cov[i][j] = 0;
    }
  }
  for (QVector3D& neighbor : nn) {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        cov[i][j] +=
            (neighbor[i] - currentPoint[i]) * (neighbor[j] - currentPoint[j]);
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      cov[i][j] /= nn.size();
    }
  }
}

double ISSDetector::computeResolution(KdTree<QVector3D>& input) {
  const std::vector<QVector3D>& surface = input.getPcRef();
  double resolution = 0.0;
  for (const QVector3D& point : surface) {
    std::vector<std::pair<float, uint32_t>> knn = input.kNNIndex(point, 2);
    resolution += std::sqrt(knn[1].first);
  }
  resolution /= surface.size();
  return resolution;
}
