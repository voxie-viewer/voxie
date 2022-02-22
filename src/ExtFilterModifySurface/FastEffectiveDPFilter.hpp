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

#ifndef FASTEFFECTIVEDPFILTER_H
#define FASTEFFECTIVEDPFILTER_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>

class FastEffectiveDPFilter {
 public:
  FastEffectiveDPFilter();
  void compute(vx::Array2<float>& vertices,
               vx::Array2<const uint32_t> triangles, double threshold,
               int iterations,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);

 private:
  double threshold;
  QList<QVector3D> filteredNormals;
  QList<QVector3D> normals;
  QList<QList<int>> neighbours;
  QList<QVector3D> differences;
  QList<QVector3D> centroids;

  void getTrianglesInfo(vx::Array2<float> vertices,
                        vx::Array2<const uint32_t> triangles);
  void filterNormals(vx::Array2<float> vertices,
                     vx::Array2<const uint32_t> triangles);
  void setUpNeighbours(vx::Array2<float> vertices,
                       vx::Array2<const uint32_t> triangles);
  void updateVertices(vx::Array2<float>& vertices,
                      vx::Array2<const uint32_t> triangles);
};

#endif  // FASTEFFECTIVEDPFILTER_H
