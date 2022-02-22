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

#ifndef IterativeClosestPoint_H
#define IterativeClosestPoint_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusTypeList.hpp>
#include "KdTree.hpp"
#include "Svd.hpp"

#include <array>

#include <QObject>

class IterativeClosestPoint {
 public:
  IterativeClosestPoint();
  void setKeyPoints(vx::Array2<const float>* vxRefKeyPoints,
                    vx::Array2<const float>* vxInKeyPoints);
  void compute(vx::Array2<const float>& vxRefSurface,
               vx::Array2<const float>& vxRefNormals,
               vx::Array2<const float>& vxInSurface,
               vx::Array2<const float>& vxInNormals, float supportRadius,
               QString method, std::tuple<double, double, double>& translation,
               std::tuple<double, double, double, double>& rotation,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void initialGuess(std::vector<QVector3D>& refPoints,
                    std::vector<QVector3D>& refKeyPoints,
                    std::vector<QVector3D>& refNormals,
                    std::vector<std::vector<float>>& refFeatures,
                    std::vector<QVector3D>& inPoints,
                    std::vector<QVector3D>& inNormals,
                    std::vector<QVector3D>& inKeyPoints,
                    std::vector<std::vector<float>>& inFeatures,
                    float angleThreshold);
  void normalSpaceSampling(vx::Array2<const float>& vertices,
                           std::vector<QVector3D>& normals,
                           std::vector<QVector3D>& sampledPoints);
  void sortIntoBuckets(const std::vector<QVector3D>& n,
                       std::vector<std::vector<int>>& buckets);
  void findCorrespondence(KdTree<QVector3D>& refTree,
                          std::vector<QVector3D>& refNormals,
                          KdTree<QVector3D>& inTree,
                          std::vector<QVector3D>& inNormals,
                          float distThreshold, float angleThreshold,
                          std::vector<QVector3D*>& inMatch, bool useRejection);
  void computeRotationMatrix(std::vector<QVector3D>& inPoints,
                             QVector3D& refMean, QVector3D& inMean,
                             std::vector<QVector3D*>& inMatch,
                             float* rotationMatrix);
  void applyTransformation(std::vector<QVector3D>& inPoints,
                           float* rotationMatrix, QVector3D& refMean,
                           QVector3D& inMean);
  double calculateRotation(float* rotationMatrix,
                           std::vector<double>& rotationAxis);
  void multMatrix(float* matrix, float& x, float& y, float& z);
  void calculateNormals(vx::Array2<const float>& vertices,
                        vx::Array2<const uint>& triangles,
                        std::vector<QVector3D>& normals);
  QVector3D calculateTranslation(QVector3D& refMean, QVector3D& inMean,
                                 float* rotationMatrix);
  QVector3D computeMean(std::vector<QVector3D>& points,
                        std::vector<QVector3D*>& match, bool isRef);
  float calculateError(std::vector<QVector3D>& inPoints,
                       std::vector<QVector3D*>& inMatch);
  float calculateThreshold(std::vector<QVector3D>& points,
                           std::vector<QVector3D*>& match);
  void testMatchNum(std::vector<QVector3D*>& inMatch);
  void setNumSamples(uint32_t numSamples) { numSamples_ = numSamples; }

 private:
  std::vector<QVector3D> refFeaturePoints_;
  std::vector<QVector3D> inFeaturePoints_;
  uint32_t numSamples_;
};

#endif  // IterativeClosestPoint_H
