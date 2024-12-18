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

#include "IterativeClosestPoint.hpp"
#include "FeatureDescriptor.hpp"

#include <QtGui/QVector3D>

#include <cmath>

// TODO: Should this be a parameter of the filter?
static const uint32_t seed = 709117;

IterativeClosestPoint::IterativeClosestPoint() : rng_(seed) {}

void IterativeClosestPoint::compute(
    vx::Array2<const float>& vxRefSurface,
    vx::Array2<const float>& vxRefNormals, vx::Array2<const float>& vxInSurface,
    vx::Array2<const float>& vxInNormals, float supportRadius, QString method,
    std::tuple<double, double, double>& translation,
    std::tuple<double, double, double, double>& rotation,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  std::vector<QVector3D> refPoints;
  std::vector<QVector3D> inPoints;
  std::vector<std::vector<float>> refFeatures;
  std::vector<std::vector<float>> inFeatures;
  std::vector<QVector3D> refNormals;
  std::vector<QVector3D> inNormals;
  std::vector<QVector3D*> copyMatch;
  std::vector<QVector3D> copyPoints;
  std::vector<QVector3D*> inMatch;
  QVector3D refMean;
  QVector3D inMean;
  QVector3D copyMean;
  float angleThreshold = std::cos(M_PI / 8);
  float rotationMatrix[9];

  // preprocessing
  {
    // copy normals
    for (uint32_t i = 0; i < vxRefNormals.size<0>(); i++) {
      refNormals.push_back(QVector3D(vxRefNormals(i, 0), vxRefNormals(i, 1),
                                     vxRefNormals(i, 2)));
    }
    for (uint32_t i = 0; i < vxInNormals.size<0>(); i++) {
      inNormals.push_back(
          QVector3D(vxInNormals(i, 0), vxInNormals(i, 1), vxInNormals(i, 2)));
    }

    // normal space subsampling
    normalSpaceSampling(vxRefSurface, refNormals, refPoints);
    normalSpaceSampling(vxInSurface, inNormals, inPoints);
    for (uint32_t i = 0; i < inPoints.size(); i++) {
      inMatch.push_back(nullptr);
      copyPoints.push_back(inPoints[i]);
      copyMatch.push_back(&inPoints[i]);
    }

    copyMean = computeMean(copyPoints, copyMatch, false);

    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.05, vx::emptyOptions()));

    if (method ==
            "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method.FPFH" ||
        method ==
            "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method.PFH") {
      // copy surfaces
      std::vector<QVector3D> refSurface;
      std::vector<QVector3D> inSurface;
      for (uint32_t i = 0; i < vxRefSurface.size<0>(); i++) {
        refSurface.push_back(QVector3D(vxRefSurface(i, 0), vxRefSurface(i, 1),
                                       vxRefSurface(i, 2)));
      }
      for (uint32_t i = 0; i < vxInSurface.size<0>(); i++) {
        inSurface.push_back(
            QVector3D(vxInSurface(i, 0), vxInSurface(i, 1), vxInSurface(i, 2)));
      }
      KdTree<QVector3D> refTree(refSurface);
      KdTree<QVector3D> inTree(inSurface);

      // calculate features
      FeatureDescriptor fd;
      fd.setSupportRadius(supportRadius);
      if (method ==
          "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method.FPFH") {
        fd.calculateFPFH(refTree, refNormals, refFeaturePoints_, refFeatures);
        fd.calculateFPFH(inTree, inNormals, inFeaturePoints_, inFeatures);
      } else {
        fd.calculatePFH(refTree, refNormals, refFeaturePoints_, refFeatures);
        fd.calculatePFH(inTree, inNormals, inFeaturePoints_, inFeatures);
      }

      initialGuess(refPoints, refNormals, refFeaturePoints_, refFeatures,
                   inPoints, inNormals, inFeaturePoints_, inFeatures,
                   angleThreshold);
    } else if (method ==
               "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method."
               "Center") {
      //     shift center of mass
      QVector3D refMean2 = computeMean(refPoints, copyMatch, false);
      QVector3D inMean2 = computeMean(inPoints, copyMatch, false);
      for (uint32_t i = 0; i < inPoints.size(); i++) {
        inPoints[i] = inPoints[i] - inMean2 + refMean2;
      }
    } else if (method ==
               "de.uni_stuttgart.Voxie.Filter.IterativeClosestPoint.Method."
               "Property") {
      inMean = QVector3D(std::get<0>(translation), std::get<1>(translation),
                         std::get<2>(translation));

      double qr = std::get<0>(rotation);
      double qi = std::get<1>(rotation);
      double qj = std::get<2>(rotation);
      double qk = std::get<3>(rotation);

      rotationMatrix[0] = 2 * (qj * qj + qk * qk);
      rotationMatrix[1] = 2 * (qi * qj - qk * qr);
      rotationMatrix[2] = 2 * (qi * qk + qj * qr);
      rotationMatrix[3] = 2 * (qi * qj + qk * qr);
      rotationMatrix[4] = 2 * (qi * qi + qk * qk);
      rotationMatrix[5] = 2 * (qj * qk - qi * qr);
      rotationMatrix[6] = 2 * (qi * qk - qj * qr);
      rotationMatrix[7] = 2 * (qj * qk + qi * qr);
      rotationMatrix[8] = 2 * (qi * qi + qj * qj);
      rotationMatrix[0] = 1 - rotationMatrix[0];
      rotationMatrix[4] = 1 - rotationMatrix[4];
      rotationMatrix[8] = 1 - rotationMatrix[8];

      // TODO: Is refMean and inMean swapped here?
      // TODO: refMean seems to be uninitialized
      applyTransformation(inPoints, rotationMatrix, inMean, refMean);
    }

    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.10, vx::emptyOptions()));
  }

  KdTree<QVector3D> inTree(inPoints);
  KdTree<QVector3D> refTree(refPoints);

  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.20, vx::emptyOptions()));

  float errorOld = 0;
  float threshold = -1;
  for (int iteration = 0; iteration < 100; iteration++) {
    // determine correspondences
    findCorrespondence(refTree, refNormals, inTree, inNormals, threshold,
                       angleThreshold, inMatch, false);

    // calculate new threshold
    threshold = calculateThreshold(inPoints, inMatch);

    refMean = computeMean(inPoints, inMatch, true);
    inMean = computeMean(inPoints, inMatch, false);

    // compute transformation params
    computeRotationMatrix(inPoints, refMean, inMean, inMatch, rotationMatrix);

    // apply transformation
    applyTransformation(inPoints, rotationMatrix, refMean, inMean);

    // calculate error
    float error = calculateError(inPoints, inMatch);
    if (std::abs(errorOld - error) / (errorOld + error + 0.00000001) <
        0.00001) {
      break;
    }
    errorOld = error;
  }
  testMatchNum(inMatch);

  inMean = computeMean(inPoints, copyMatch, false);
  computeRotationMatrix(copyPoints, inMean, copyMean, copyMatch,
                        rotationMatrix);
  std::vector<double> rotationAxis;
  double angle = calculateRotation(rotationMatrix, rotationAxis);

  // write results
  double ca = std::cos(angle / 2);
  double sa = std::sin(angle / 2);
  rotation = std::tuple<double, double, double, double>{
      ca, rotationAxis.at(0) * sa, rotationAxis.at(1) * sa,
      rotationAxis.at(2) * sa};
  QVector3D tResult = calculateTranslation(inMean, copyMean, rotationMatrix);
  translation =
      std::tuple<double, double, double>{tResult.x(), tResult.y(), tResult.z()};
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(1.00, vx::emptyOptions()));
}

void IterativeClosestPoint::testMatchNum(std::vector<QVector3D*>& inMatch) {
  int matches = 0;
  for (uint32_t i = 0; i < inMatch.size(); i++) {
    if (inMatch[i] != nullptr) {
      matches++;
    }
  }
  // qDebug() << matches;
  if (matches <= 4) {
    qDebug() << "Warning: " << matches
             << " matches found. Small number matched pairs can lead to "
                "incorrect results.";
  }
}

void IterativeClosestPoint::setKeyPoints(
    vx::Array2<const float>* vxRefKeyPoints,
    vx::Array2<const float>* vxInKeyPoints) {
  // copy key points
  for (uint32_t i = 0; i < vxRefKeyPoints->size<0>(); i++) {
    refFeaturePoints_.push_back(QVector3D((*vxRefKeyPoints)(i, 0),
                                          (*vxRefKeyPoints)(i, 1),
                                          (*vxRefKeyPoints)(i, 2)));
  }
  for (uint32_t i = 0; i < vxInKeyPoints->size<0>(); i++) {
    inFeaturePoints_.push_back(QVector3D((*vxInKeyPoints)(i, 0),
                                         (*vxInKeyPoints)(i, 1),
                                         (*vxInKeyPoints)(i, 2)));
  }
}

float IterativeClosestPoint::calculateError(std::vector<QVector3D>& inPoints,
                                            std::vector<QVector3D*>& inMatch) {
  float error = 0;
  for (uint32_t i = 0; i < inPoints.size(); i++) {
    if (inMatch[i] != nullptr) {
      error += inPoints[i].distanceToPoint(*inMatch[i]);
    }
  }
  return error;
}

void IterativeClosestPoint::initialGuess(
    std::vector<QVector3D>& refPoints, std::vector<QVector3D>& refNormals,
    std::vector<QVector3D>& refKeyPoints,
    std::vector<std::vector<float>>& refFeatures,
    std::vector<QVector3D>& inPoints, std::vector<QVector3D>& inNormals,
    std::vector<QVector3D>& inKeyPoints,
    std::vector<std::vector<float>>& inFeatures, float angleTreshold) {
  if (refFeatures.size() == 0 || inFeatures.size() == 0) {
    return;
  }

  // match features
  std::vector<QVector3D*> inKeyMatch;
  KdTree<std::vector<float>> refKeyTree(refFeatures);
  KdTree<std::vector<float>> inKeyTree(inFeatures);
  for (uint32_t i = 0; i < inKeyPoints.size(); i++) {
    // KD tree can only return nearestNeighbor
    uint32_t index = refKeyTree.nearestNeighborIndex(inFeatures[i]);
    QVector3D* match = &refKeyPoints[index];
    // only match if ref is NN of in and in is NN of ref
    if (inKeyTree.nearestNeighborIndex(refFeatures[index]) != i) {
      match = nullptr;
    }
    inKeyMatch.push_back(match);
  }

  // safe valid indices
  std::vector<QVector3D*> validMatch;
  std::vector<QVector3D> validInPoints;
  for (uint32_t i = 0; i < inKeyMatch.size(); i++) {
    if (inKeyMatch[i] != nullptr) {
      validMatch.push_back(inKeyMatch[i]);
      validInPoints.push_back(inKeyPoints[i]);
    }
  }
  if (validMatch.size() < 3) {
    return;
  }

  // RANSAC: randomly pick matches, evaluate, take best random set
  // p = probability, e = outlierratio, s = samples needed to estimate model
  // T = log(1-p)/log(1-(1-e)^s)
  float p = 0.999, e = 0.99, s = 3;
  int iterations = std::log(1 - p) / std::log(1 - std::pow(1 - e, s));
  std::vector<QVector3D*> randMatch(validMatch.size(), nullptr);
  int maxInlierCount = 0;
  float bestRotation[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
  QVector3D bestRefMean(0, 0, 0);
  QVector3D bestInMean(0, 0, 0);
  float rotationMatrix[9];

  std::vector<QVector3D*> inMatch;
  std::vector<QVector3D> inPointsCopy;
  for (uint32_t i = 0; i < inPoints.size(); i++) {
    inMatch.push_back(nullptr);
    inPointsCopy.push_back(inPoints[i]);
  }

  KdTree<QVector3D> refTree(refPoints);
  KdTree<QVector3D> inTree(inPointsCopy);

  int cMatches = 0;
  for (int iteration = 0; iteration < iterations; iteration++) {
    // pick random correspondences
    int n1 = rand() % validMatch.size();
    int n2 = rand() % validMatch.size();
    while (n2 == n1) {
      n2 = rand() % validMatch.size();
    }
    int n3 = rand() % validMatch.size();
    while (n3 == n1 || n3 == n2) {
      n3 = rand() % validMatch.size();
    }
    // evaluate matches (3 randomly picked matches have to be consistent)
    QVector3D& p1 = validInPoints[n1];
    QVector3D* q1 = validMatch[n1];
    QVector3D& p2 = validInPoints[n2];
    QVector3D* q2 = validMatch[n2];
    QVector3D& p3 = validInPoints[n3];
    QVector3D* q3 = validMatch[n3];
    float refDist12 = (*q1).distanceToPoint((*q2));
    float inDist12 = p1.distanceToPoint(p2);
    float refDist23 = (*q2).distanceToPoint((*q3));
    float inDist23 = p2.distanceToPoint(p3);
    float refDist13 = (*q1).distanceToPoint((*q3));
    float inDist13 = p1.distanceToPoint(p3);
    if (std::abs(inDist12 - refDist12) / (inDist12 + refDist12 + 0.00000001) >
        0.01) {
      continue;
    }
    if (std::abs(inDist23 - refDist23) / (inDist23 + refDist23 + 0.00000001) >
        0.01) {
      continue;
    }
    if (std::abs(inDist13 - refDist13) / (inDist13 + refDist13 + 0.00000001) >
        0.01) {
      continue;
    }

    // TODO replace? not very efficient
    // safe computation time, evaluate at most 1000 times
    cMatches++;
    if (cMatches == 1000) {
      break;
    }
    randMatch[n1] = validMatch[n1];
    randMatch[n2] = validMatch[n2];
    randMatch[n3] = validMatch[n3];

    // compute transformation params
    QVector3D refMean = computeMean(validInPoints, randMatch, true);
    QVector3D inMean = computeMean(validInPoints, randMatch, false);
    computeRotationMatrix(validInPoints, refMean, inMean, randMatch,
                          rotationMatrix);

    // apply params
    for (uint32_t i = 0; i < inPointsCopy.size(); ++i) {
      inPointsCopy[i].setX(inPoints[i].x());
      inPointsCopy[i].setY(inPoints[i].y());
      inPointsCopy[i].setZ(inPoints[i].z());
    }
    applyTransformation(inPointsCopy, rotationMatrix, refMean, inMean);
    findCorrespondence(refTree, refNormals, inTree, inNormals, -1,
                       angleTreshold, inMatch, true);

    // evaluate params
    int inlierCount = 0;
    for (uint32_t i = 0; i < inPointsCopy.size(); ++i) {
      if (inMatch[i] == nullptr) {
        continue;
      }
      QVector3D& pi = inPoints[i];
      float inDist = p1.distanceToPoint(pi);
      QVector3D* qi = inMatch[i];
      float refDist = (*q1).distanceToPoint((*qi));
      if (std::abs(inDist - refDist) / (inDist + refDist + 0.000001) < 0.01) {
        inlierCount++;
      }
    }
    if (inlierCount > maxInlierCount) {
      maxInlierCount = inlierCount;
      for (int i = 0; i < 9; i++) {
        bestRotation[i] = rotationMatrix[i];
      }
      bestRefMean = refMean;
      bestInMean = inMean;
    }

    randMatch[n1] = nullptr;
    randMatch[n2] = nullptr;
    randMatch[n3] = nullptr;
  }
  qDebug() << cMatches << " consistent RANSAC matches";

  // apply initial guess
  applyTransformation(inPoints, bestRotation, bestRefMean, bestInMean);
}

void IterativeClosestPoint::calculateNormals(vx::Array2<const float>& vertices,
                                             vx::Array2<const uint>& triangles,
                                             std::vector<QVector3D>& normals) {
  for (uint32_t i = 0; i < triangles.size<0>(); i++) {
    uint aInd = triangles(i, 0);
    uint bInd = triangles(i, 1);
    uint cInd = triangles(i, 2);
    QVector3D a(vertices(aInd, 0), vertices(aInd, 1), vertices(aInd, 2));
    QVector3D b(vertices(bInd, 0), vertices(bInd, 1), vertices(bInd, 2));
    QVector3D c(vertices(cInd, 0), vertices(cInd, 1), vertices(cInd, 2));
    QVector3D normal = QVector3D::crossProduct(b - c, c - a);
    normals[aInd] += normal;
    normals[bInd] += normal;
    normals[cInd] += normal;
  }
  for (uint32_t i = 0; i < normals.size(); i++) {
    if (normals[i].length() != 0) {
      normals[i] /= normals[i].length();
    }
  }
}

void IterativeClosestPoint::findCorrespondence(
    KdTree<QVector3D>& refTree, std::vector<QVector3D>& refNormals,
    KdTree<QVector3D>& inTree, std::vector<QVector3D>& inNormals,
    float distThreshold, float angleThreshold, std::vector<QVector3D*>& inMatch,
    bool useRejection) {
  std::vector<QVector3D>& inPoints = inTree.getPcRef();
  std::vector<QVector3D>& refPoints = refTree.getPcRef();
  std::vector<float> distances(refPoints.size(), -1);
  uint32_t* indexArray = new uint32_t[refPoints.size()];
  int refIndex;
  for (uint32_t i = 0; i < inPoints.size(); ++i) {
    inMatch[i] = nullptr;
    if (refTree.nearestNeighborIndex(inPoints[i], distThreshold, refIndex)) {
      // rejection:
      // normals have to match
      if (useRejection) {
        double cosAngle = inNormals[i][0] * refNormals[refIndex][0];
        cosAngle += inNormals[i][1] * refNormals[refIndex][1];
        cosAngle += inNormals[i][2] * refNormals[refIndex][2];
        // should orientation matter?
        if (std::abs(cosAngle) > angleThreshold) {
          continue;
        }

        // keep only unique/best matches
        float dist = refPoints[refIndex].distanceToPoint(inPoints[i]);
        if (dist >= distances[refIndex] && distances[refIndex] != -1) {
          continue;
        }
        if (dist < distances[refIndex]) {
          // remove previous best match
          inMatch[indexArray[refIndex]] = nullptr;
        }

        distances[refIndex] = dist;
        indexArray[refIndex] = i;
      }

      inMatch[i] = &refPoints[refIndex];
    }
  }
  delete[] indexArray;
}

void IterativeClosestPoint::computeRotationMatrix(
    std::vector<QVector3D>& inPoints, QVector3D& refMean, QVector3D& inMean,
    std::vector<QVector3D*>& inMatch, float* rotationMatrix) {
  float h[3][3] = {};  // TODO: necessary?
  float* ph[3];        // array of pointer
  ph[0] = h[0];
  ph[1] = h[1];
  ph[2] = h[2];
  float w[3];
  float v[3][3];
  float* pv[3];  // array of pointer
  pv[0] = v[0];
  pv[1] = v[1];
  pv[2] = v[2];
  for (uint32_t pIndex = 0; pIndex < inPoints.size(); pIndex++) {
    if (inMatch[pIndex] == nullptr) {
      continue;
    }
    QVector3D inP = inPoints[pIndex] - inMean;
    QVector3D refP = *inMatch[pIndex] - refMean;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        h[i][j] += inP[j] * refP[i];
      }
    }
  }
  dsvd(ph, 3, 3, w, pv);  // h = UDV^T, U -> h, V -> v
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      float dotProd = 0;
      // R = UV^T
      for (int k = 0; k < 3; k++) {
        dotProd += h[i][k] * v[j][k];
      }
      rotationMatrix[j + i * 3] = std::round(dotProd * 1000000) / 1000000;
    }
  }
}

// TODO: Why are inPoints, refMean and inMean passed as non-const refs when they
// are not modified?
void IterativeClosestPoint::applyTransformation(
    std::vector<QVector3D>& inPoints, float* rotationMatrix, QVector3D& refMean,
    QVector3D& inMean) {
  for (uint32_t pIndex = 0; pIndex < inPoints.size(); pIndex++) {
    QVector3D& inP = inPoints.at(pIndex);
    inP -= inMean;
    float x = inP.x();
    float y = inP.y();
    float z = inP.z();
    multMatrix(rotationMatrix, x, y, z);
    inP.setX(x);
    inP.setY(y);
    inP.setZ(z);
    inP += refMean;
  }
}

QVector3D IterativeClosestPoint::calculateTranslation(QVector3D& refMean,
                                                      QVector3D& inMean,
                                                      float* rotationMatrix) {
  float x = inMean.x();
  float y = inMean.y();
  float z = inMean.z();
  multMatrix(rotationMatrix, x, y, z);
  return refMean - QVector3D(x, y, z);
}

double IterativeClosestPoint::calculateRotation(
    float* rotationMatrix, std::vector<double>& rotationAxis) {
  double alpha = 0;
  // not symmetric ? (R-R^T)u=0 : Tr(R)=1+2cos(alpha)
  double ux = 1;
  double uy = 0;
  double uz = 0;
  float trace = rotationMatrix[0] + rotationMatrix[4] + rotationMatrix[8];
  float arg = (trace - 1) / 2;
  if (arg > 1) {
    arg = 1;
  } else if (arg < -1) {
    arg = -1;
  }
  // can flip sign
  double cosAlpha = std::acos(arg);
  if (std::abs(rotationMatrix[1] - rotationMatrix[3]) > 0.00001 ||
      std::abs(rotationMatrix[2] - rotationMatrix[6]) > 0.00001 ||
      std::abs(rotationMatrix[5] - rotationMatrix[7]) > 0.00001) {
    double x = (rotationMatrix[7] - rotationMatrix[5]) / 2;
    double y = (rotationMatrix[2] - rotationMatrix[6]) / 2;
    double z = (rotationMatrix[3] - rotationMatrix[1]) / 2;
    double len = std::sqrt(x * x + y * y + z * z);
    // can be off by pi
    alpha = std::asin(len);
    // TODO: condition is unsafe, numerical error happend to be as large as
    // 0.0005 find better way to check whether shift angle by PI or not
    if (std::abs(std::abs(cosAlpha) - std::abs(alpha)) > 0.001) {
      alpha = M_PI - alpha;
    }

    if (alpha != 0) {
      ux = x / len;
      uy = y / len;
      uz = z / len;
    }
  } else {
    if (arg != 1) {
      ux = std::sqrt((rotationMatrix[0] - arg) / (1 - arg));
      uy = std::sqrt((rotationMatrix[4] - arg) / (1 - arg));
      uz = std::sqrt((rotationMatrix[8] - arg) / (1 - arg));
    }
  }
  rotationAxis.push_back(ux);
  rotationAxis.push_back(uy);
  rotationAxis.push_back(uz);
  return alpha;
}

void IterativeClosestPoint::multMatrix(float* matrix, float& x, float& y,
                                       float& z) {
  float xHelp = x;
  float yHelp = y;
  float zHelp = z;

  x = matrix[0] * xHelp;
  x += matrix[1] * yHelp;
  x += matrix[2] * zHelp;

  y = matrix[3] * xHelp;
  y += matrix[4] * yHelp;
  y += matrix[5] * zHelp;

  z = matrix[6] * xHelp;
  z += matrix[7] * yHelp;
  z += matrix[8] * zHelp;
}

void IterativeClosestPoint::normalSpaceSampling(
    vx::Array2<const float>& vertices, std::vector<QVector3D>& normals,
    std::vector<QVector3D>& sampledPoints) {
  if (numSamples_ == 0) {
    for (uint32_t ind = 0; ind < vertices.size<0>(); ind++) {
      sampledPoints.push_back(
          QVector3D(vertices(ind, 0), vertices(ind, 1), vertices(ind, 2)));
    }
    return;
  }
  uint32_t numSamples = std::min(numSamples_, (uint32_t)vertices.size<0>());
  std::vector<std::vector<int>> normbuckets;
  sortIntoBuckets(normals, normbuckets);
  std::vector<QVector3D> sampledNormals;
  while (sampledPoints.size() < numSamples) {
    bool bucketsEmpty = true;
    for (uint i = 0; i < normbuckets.size(); i++) {
      if (!normbuckets[i].empty()) {
        bucketsEmpty = false;
        int ind = normbuckets[i].back();
        sampledPoints.push_back(
            QVector3D(vertices(ind, 0), vertices(ind, 1), vertices(ind, 2)));
        sampledNormals.push_back(normals[ind]);
        normbuckets[i].pop_back();
      }
    }
    if (bucketsEmpty) {
      break;
    }
  }
  normals = sampledNormals;
}

void IterativeClosestPoint::sortIntoBuckets(
    const std::vector<QVector3D>& n, std::vector<std::vector<int>>& buckets) {
  // use for each possible dominant axis, use nBucket*nBucket buckets
  const int nBucket = 4;
  buckets.resize(3 * nBucket * nBucket);
  // round up sqrt(2) to ensure later that i/j is between 0 and nBucket-1
  const float norm = nBucket / 1.414214f;
  for (uint ind = 0; ind < n.size(); ind++) {
    const QVector3D& N = n[ind];
    if (N[0] == 0 && N[1] == 0 && N[2] == 0) {
      // no normal information
      continue;
    }
    float ax = std::abs(N[0]), ay = std::abs(N[1]), az = std::abs(N[2]);
    int k;
    float u, v;
    if (ax > ay && ax > az) {
      k = 0;
      u = (N[0] > 0) ? N[1] : -N[1];
      v = (N[0] > 0) ? N[2] : -N[2];
    } else if (ay > ax && ay > az) {
      k = 1;
      u = (N[1] > 0) ? N[2] : -N[2];
      v = (N[1] > 0) ? N[0] : -N[0];
    } else {
      k = 2;
      u = (N[2] > 0) ? N[0] : -N[0];
      v = (N[2] > 0) ? N[1] : -N[1];
    }
    // smalles possible value for u(v) is -sqrt(2)/2, normalize and shift to [0,
    // nBucket-1]
    int i = int(u * norm) + (nBucket / 2);
    int j = int(v * norm) + (nBucket / 2);
    buckets[i + j * nBucket + k * nBucket * nBucket].push_back(ind);
  }
  for (uint bucket = 0; bucket < buckets.size(); bucket++) {
    std::shuffle(buckets[bucket].begin(), buckets[bucket].end(), rng_);
  }
}

QVector3D IterativeClosestPoint::computeMean(std::vector<QVector3D>& points,
                                             std::vector<QVector3D*>& match,
                                             bool isRef) {
  QVector3D mean(0, 0, 0);
  int count = 0;

  for (uint32_t i = 0; i < match.size(); i++) {
    if (match[i] != nullptr) {
      if (isRef) {
        mean += *match[i];
      } else {
        mean += points[i];
      }
      count++;
    }
  }
  mean /= count;
  return mean;
}

float IterativeClosestPoint::calculateThreshold(
    std::vector<QVector3D>& points, std::vector<QVector3D*>& match) {
  double mean = 0;
  int count = 0;
  for (uint32_t i = 0; i < match.size(); i++) {
    if (match[i] != nullptr) {
      mean += points[i].distanceToPoint(*match[i]);
      count++;
    }
  }
  if (count == 0) {
    return -1;
  }
  mean /= count;

  double stdDev = 0;
  for (uint32_t i = 0; i < match.size(); i++) {
    double value = 0;
    if (match[i] != nullptr) {
      value = points[i].distanceToPoint(*match[i]) - mean;
      stdDev += value * value;
    }
  }
  stdDev = std::sqrt(stdDev / count);
  // mean + 3stdDev worked best
  return mean + 3 * stdDev;
}
