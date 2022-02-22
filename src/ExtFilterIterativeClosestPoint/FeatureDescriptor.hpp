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

#ifndef FEATUREDESCRIPTOR_HPP
#define FEATUREDESCRIPTOR_HPP

#include <VoxieClient/Array.hpp>
#include "KdTree.hpp"

class FeatureDescriptor {
 public:
  FeatureDescriptor() {}
  void calculateFPFH(KdTree<QVector3D>& surfaceTree,
                     std::vector<QVector3D>& normals,
                     std::vector<QVector3D>& keyPoints,
                     std::vector<std::vector<float>>& features);
  void calculatePFH(KdTree<QVector3D>& surfaceTree,
                    std::vector<QVector3D>& normals,
                    std::vector<QVector3D>& keyPoints,
                    std::vector<std::vector<float>>& features);
  float calcSupportRadius(KdTree<QVector3D>& surfaceTree);
  void setSupportRadius(float radius) { supportRadius_ = radius; }
  double computeResolution(KdTree<QVector3D>& surfaceTree);

 private:
  float supportRadius_ = 0;
  float resolutionMultiplier_ = 6;
  int binSizeFPFH_ = 11;
  int binSizePFH_ = 5;

  void calculateSparseFPFH(KdTree<QVector3D>& surfaceTree,
                           std::vector<QVector3D>& normals,
                           std::vector<QVector3D>& keyPoints, float radius,
                           std::vector<std::vector<float>>& features);
  void calculateDenseFPFH(KdTree<QVector3D>& surfaceTree,
                          std::vector<QVector3D>& normals,
                          std::vector<QVector3D>& keyPoints, float radius,
                          std::vector<std::vector<float>>& features);
  bool calculatePairFeature(const QVector3D& pi, const QVector3D& ni,
                            const QVector3D& pj, const QVector3D& nj,
                            float features[3]);
  void normalizeFeatureHistogram(std::vector<std::vector<float>>& features,
                                 int numFeatures);
};

#endif  // FEATUREDESCRIPTOR_HPP
