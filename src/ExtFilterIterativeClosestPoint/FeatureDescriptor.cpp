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

#include "FeatureDescriptor.hpp"

#include <QDebug>
#include <cmath>

#define PI 3.141593

void FeatureDescriptor::calculateFPFH(
    KdTree<QVector3D>& surfaceTree, std::vector<QVector3D>& normals,
    std::vector<QVector3D>& keyPoints,
    std::vector<std::vector<float>>& features) {
  uint numBins = 3 * binSizeFPFH_;
  features = std::vector<std::vector<float>>(keyPoints.size(),
                                             std::vector<float>(numBins, 0));
  float radius = calcSupportRadius(surfaceTree);

  // estimate NN size
  int nnSize = 0;
  for (int i = 0; i < 5; i++) {
    int keyIndex = rand() % keyPoints.size();
    nnSize += surfaceTree.radiusSearchIndex(keyPoints[keyIndex], radius).size();
  }
  nnSize /= 5;

  uint32_t sparseSize = keyPoints.size() * nnSize * nnSize;
  uint32_t denseSize = normals.size() * nnSize + keyPoints.size() * nnSize;

  if (sparseSize < denseSize) {
    calculateSparseFPFH(surfaceTree, normals, keyPoints, radius, features);
  } else {
    calculateDenseFPFH(surfaceTree, normals, keyPoints, radius, features);
  }
}

// For non dense features, (slower than PFH)
//  1. For each feature point iterate over each neighbor and calculate SPFH
//       O(N_feat * K_nn * K_nn)
void FeatureDescriptor::calculateSparseFPFH(
    KdTree<QVector3D>& surfaceTree, std::vector<QVector3D>& normals,
    std::vector<QVector3D>& keyPoints, float radius,
    std::vector<std::vector<float>>& features) {
  std::vector<QVector3D> surface = surfaceTree.getPcRef();
  uint32_t numBins = 3 * binSizeFPFH_;
  float currentFeatures[3];

  for (uint32_t i = 0; i < keyPoints.size(); i++) {
    std::vector<uint32_t> pointNeighbors =
        surfaceTree.radiusSearchIndex(keyPoints[i], radius);
    std::vector<std::vector<float>> pointFeatures;

    // calculate SPFH feature for each neighbor
    for (uint32_t j = 0; j < pointNeighbors.size(); j++) {
      uint32_t indexPointNeighbor = pointNeighbors[j];
      std::vector<uint32_t> neighborsOfNeighbor =
          surfaceTree.radiusSearchIndex(surface[indexPointNeighbor], radius);
      pointFeatures.push_back(std::vector<float>(numBins, 0));

      // calculate SPFH feature
      for (uint32_t k = 0; k < neighborsOfNeighbor.size(); k++) {
        uint32_t indexNeighborOfNeighbor = neighborsOfNeighbor[k];

        if (!calculatePairFeature(
                surface[indexPointNeighbor], normals[indexPointNeighbor],
                surface[indexNeighborOfNeighbor],
                normals[indexNeighborOfNeighbor], currentFeatures)) {
          continue;
        }

        // divide into bins
        int alphaBin = int(binSizeFPFH_ * currentFeatures[0]);
        int phiBin = int(binSizeFPFH_ * currentFeatures[1]) + binSizeFPFH_;
        int thetaBin =
            int(binSizeFPFH_ * currentFeatures[2]) + 2 * binSizeFPFH_;

        pointFeatures[j][alphaBin]++;
        pointFeatures[j][phiBin]++;
        pointFeatures[j][thetaBin]++;
      }
    }

    // normalize SPFH features (each feature seperatly)
    normalizeFeatureHistogram(pointFeatures, 3);

    // calculate FPFH features
    for (uint32_t j = 0; j < pointNeighbors.size(); j++) {
      uint32_t indexPointNeighbor = pointNeighbors[j];
      // weight in range [0.5, 1]
      float weight =
          1 - keyPoints[i].distanceToPoint(surface[indexPointNeighbor]) /
                  (2 * radius);
      if (indexPointNeighbor != i) {
        weight = weight / (pointNeighbors.size() - 1);
      }
      for (uint32_t k = 0; k < numBins; k++) {
        features[i][k] += pointFeatures[j][k] * weight;
      }
    }
  }

  // normalize FPFH features (each feature seperatly)
  normalizeFeatureHistogram(features, 3);

  // normalize FPFH features
  normalizeFeatureHistogram(features, 1);
}

// If #features close to #surfacePoints then this is faster than FPFH and PFH
//  1. Iterate over all points on surface to calculate SPFH for each surface
//  point
//       O(N_surf * K_nn)
//  2. Iterate over all feature points to calculate FPFH
//       O(N_feat * K_nn)
void FeatureDescriptor::calculateDenseFPFH(
    KdTree<QVector3D>& surfaceTree, std::vector<QVector3D>& normals,
    std::vector<QVector3D>& keyPoints, float radius,
    std::vector<std::vector<float>>& features) {
  std::vector<QVector3D> surface = surfaceTree.getPcRef();
  uint32_t numBins = 3 * binSizeFPFH_;
  float currentFeatures[3];
  std::vector<std::vector<float>> tmpFeatures = std::vector<std::vector<float>>(
      surface.size(), std::vector<float>(numBins, 0));

  // calculate SPFH feature for each point
  for (uint32_t i = 1; i < surface.size(); i++) {
    std::vector<uint32_t> pointNeighbors =
        surfaceTree.radiusSearchIndex(surface[i], radius);

    // calculate SPFH feature
    for (uint32_t j = 0; j < pointNeighbors.size(); j++) {
      uint32_t indexPointNeighbor = pointNeighbors[j];
      if (i <= indexPointNeighbor) {
        continue;
      }

      if (!calculatePairFeature(surface[i], normals[i],
                                surface[indexPointNeighbor],
                                normals[indexPointNeighbor], currentFeatures)) {
        continue;
      }

      // divide into bins
      int alphaBin = int(binSizeFPFH_ * currentFeatures[0]);
      int phiBin = int(binSizeFPFH_ * currentFeatures[1]) + binSizeFPFH_;
      int thetaBin = int(binSizeFPFH_ * currentFeatures[2]) + 2 * binSizeFPFH_;

      tmpFeatures[i][alphaBin]++;
      tmpFeatures[i][phiBin]++;
      tmpFeatures[i][thetaBin]++;

      tmpFeatures[indexPointNeighbor][alphaBin]++;
      tmpFeatures[indexPointNeighbor][phiBin]++;
      tmpFeatures[indexPointNeighbor][thetaBin]++;
    }
  }

  // normalize SPFH features (each feature seperatly)
  normalizeFeatureHistogram(tmpFeatures, 3);

  // calculate FPFH features
  for (uint32_t i = 0; i < keyPoints.size(); i++) {
    std::vector<uint32_t> pointNeighbors =
        surfaceTree.radiusSearchIndex(keyPoints[i], radius);

    for (uint32_t j = 0; j < pointNeighbors.size(); j++) {
      uint32_t indexPointNeighbor = pointNeighbors[j];
      // weight in range [0.5, 1]
      float weight =
          1 - keyPoints[i].distanceToPoint(surface[indexPointNeighbor]) /
                  (2 * radius);
      if (indexPointNeighbor != i) {
        weight = weight / (pointNeighbors.size() - 1);
      }
      for (uint32_t k = 0; k < numBins; k++) {
        features[i][k] += tmpFeatures[indexPointNeighbor][k] * weight;
      }
    }
  }

  // normalize FPFH features (each feature seperatly)
  normalizeFeatureHistogram(features, 3);

  // normalize FPFH features
  normalizeFeatureHistogram(features, 1);
}

void FeatureDescriptor::calculatePFH(
    KdTree<QVector3D>& surfaceTree, std::vector<QVector3D>& normals,
    std::vector<QVector3D>& keyPoints,
    std::vector<std::vector<float>>& features) {
  const std::vector<QVector3D>& surface = surfaceTree.getPcRef();

  float radius = calcSupportRadius(surfaceTree);
  uint numBins = binSizePFH_ * binSizePFH_ * binSizePFH_;
  float currentFeatures[3];
  features = std::vector<std::vector<float>>(keyPoints.size(),
                                             std::vector<float>(numBins, 0));

  // calculate PFH features
  for (uint32_t i = 0; i < keyPoints.size(); i++) {
    std::vector<uint32_t> neighbors =
        surfaceTree.radiusSearchIndex(keyPoints[i], radius);
    for (uint32_t j = 0; j < neighbors.size(); j++) {
      uint32_t sInd1 = neighbors[j];
      for (uint32_t k = j + 1; k < neighbors.size(); k++) {
        uint32_t sInd2 = neighbors[k];
        if (!calculatePairFeature(surface[sInd1], normals[sInd1],
                                  surface[sInd2], normals[sInd2],
                                  currentFeatures)) {
          continue;
        }

        // divide into bins
        int alphaBin = int(binSizePFH_ * currentFeatures[0]);
        int phiBin = int(binSizePFH_ * currentFeatures[1]) * binSizePFH_;
        int thetaBin =
            int(binSizePFH_ * currentFeatures[2]) * binSizePFH_ * binSizePFH_;

        features[i][alphaBin + phiBin + thetaBin]++;
      }
    }
  }

  normalizeFeatureHistogram(features, 1);
}

void FeatureDescriptor::normalizeFeatureHistogram(
    std::vector<std::vector<float>>& features, int numFeatures) {
  if (features.size() == 0) {
    return;
  }
  uint32_t numBins = features[0].size();
  uint32_t binSize = numBins / numFeatures;
  for (uint32_t i = 0; i < features.size(); i++) {
    int binShift = 0;
    for (int feat = 0; feat < numFeatures; feat++) {
      double len = 0;
      for (uint32_t k = 0; k < binSize; k++) {
        len += features[i][k + binShift] * features[i][k + binShift];
      }
      if (len != 0) {
        len = std::sqrt(len);
        for (uint32_t k = 0; k < binSize; k++) {
          features[i][k + binShift] /= len;
        }
      }
      binShift += binSize;
    }
  }
}

bool FeatureDescriptor::calculatePairFeature(const QVector3D& pi,
                                             const QVector3D& ni,
                                             const QVector3D& pj,
                                             const QVector3D& nj,
                                             float features[3]) {
  QVector3D d = pj - pi;
  if (d.length() == 0) {
    return false;
  }
  // make sure the feature is symmetric, SPFH(p,q)=SPFH(q,p)
  float phi = QVector3D::dotProduct(ni, d) / d.length();
  float phi2 = QVector3D::dotProduct(nj, d) / d.length();
  QVector3D ni2 = ni;
  QVector3D nj2 = nj;
  if (std::abs(phi) < std::abs(phi2)) {
    ni2 = nj;
    nj2 = ni;
    d *= (-1);
    phi = -phi2;
  }

  QVector3D v = QVector3D::crossProduct(d, ni2);
  if (v.length() == 0) {
    return false;
  }
  v /= v.length();
  QVector3D w = QVector3D::crossProduct(ni2, v);
  float alpha = QVector3D::dotProduct(v, nj2);
  float theta = std::atan2(QVector3D::dotProduct(w, nj2),
                           QVector3D::dotProduct(ni2, nj2));

  // return normalized features in range [0,1)
  features[0] = (alpha + 1) / 2;
  features[1] = (phi + 1) / 2;
  features[2] = (theta + PI) / (2 * PI);
  for (int i = 0; i < 3; i++) {
    if (features[i] < 0) {
      features[i] = 0;
    } else if (features[i] >= 1) {
      features[i] = 0.9999;
    }
  }
  return true;
}

float FeatureDescriptor::calcSupportRadius(KdTree<QVector3D>& surfaceTree) {
  if (supportRadius_ <= 0) {
    float radius = resolutionMultiplier_ * computeResolution(surfaceTree);
    qDebug() << "supportRadius: " << radius;
    return radius;
  } else {
    return supportRadius_;
  }
}

double FeatureDescriptor::computeResolution(KdTree<QVector3D>& surfaceTree) {
  const std::vector<QVector3D>& surface = surfaceTree.getPcRef();
  double resolution = 0.0;
  for (const QVector3D& point : surface) {
    std::vector<std::pair<float, uint32_t>> knn =
        surfaceTree.kNNIndex(point, 2);
    resolution += std::sqrt(knn[1].first);
  }
  resolution /= surface.size();
  return resolution;
}
