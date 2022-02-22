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

#include <QObject>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>

class AddNormalAttribute {
 public:
  AddNormalAttribute();
  void compute(vx::Array2<const float>& vxSurface,
               vx::Array2<const uint>& vxTriangles,
               vx::Array2<float>& outputNormals,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void compute(vx::Array2<const float>& vxSurface,
               vx::Array3<const float>& vxVolume, QVector3D& spacing,
               QVector3D& origin, vx::Array2<float>& outputNormals,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  float sampleVolume(float x, float y, float z, int nx, int ny, int nz,
                     const float* volume);

  int getIndex(int x, int y, int z, int nx, int ny);
};

#endif  // IterativeClosestPoint_H
