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

#include "ExtractKeyPoints.hpp"

#include <QtGui/QVector3D>

ExtractKeyPoints::ExtractKeyPoints() {}

void ExtractKeyPoints::compute(
    vx::Array2<const float>& vxSurface, std::vector<QVector3D>& outPoints,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  std::vector<QVector3D> points;
  for (uint32_t i = 0; i < vxSurface.size<0>(); i++) {
    points.push_back(
        QVector3D(vxSurface(i, 0), vxSurface(i, 1), vxSurface(i, 2)));
  }
  KdTree<QVector3D> tree(points);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.05, vx::emptyOptions()));

  outPoints = iss_.detectKeypoints(tree);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(1.00, vx::emptyOptions()));
}

void ExtractKeyPoints::setParamsISS(float searchRadius, float maxRadius,
                                    float gamma21, float gamma32,
                                    int minNeighbors) {
  iss_.setSalientRadius(searchRadius);
  iss_.setNonMaxRadius(maxRadius);
  iss_.setThreshold21(gamma21);
  iss_.setThreshold32(gamma32);
  iss_.setMinNeighbors(minNeighbors);
}
