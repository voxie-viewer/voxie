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

#ifndef VOLUMEREGISTRATION_H
#define VOLUMEREGISTRATION_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusTypeList.hpp>

#include <array>
#include <cmath>
#include <complex>
#include <iostream>

#include <QObject>

class VolumeRegistration {
 public:
  VolumeRegistration();
  void compute(vx::Array3<const float>& vxRefVolume,
               vx::Array3<const float>& vxInVolume, QVector3D spacing1,
               QVector3D spacing2, QVector3D origin1, QVector3D origin2,
               std::tuple<double, double, double>& translation,
               std::tuple<double, double, double, double>& rotation,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);

  template <typename T>
  void periodicShift(int dx, int dy, int dz, int nx, int ny, int nz, T* data);
  template <typename T>
  void periodicShift2(int dx, int dy, int dz, int nx, int ny, int nz, T* data);
  template <typename T>
  void periodicShift3(int dx, int dy, int dz, int nx, int ny, int nz, T* data);

  std::vector<int> findBestShift(int nx, int ny, int nz, float* inVolume,
                                 float* help, int dx, int dy, int dz);
  void rotateVolume(int nx, int ny, int nz, std::vector<double> center,
                    std::vector<double> center2,
                    std::vector<double> rotationAxis, float angle,
                    float* inVolume, float* outVolume);
  std::vector<double> findRotationAxis(int nx, int ny, int nz, float* volume1,
                                       float* volume2);
  float findRotationAngle(int nx, int ny, int nz,
                          std::vector<double> rotationAxis, float* vol1,
                          float* vol2);
  void fillRotationMatrix(float axisX, float axisY, float axisZ, float angle,
                          float* matrix);
  void multMatrix(float* matrix, float& x, float& y, float& z);
  float sampleVolume(float x, float y, float z, int nx, int ny, int nz,
                     const float* volume);
  int getIndex(int x, int y, int z, int nx, int ny);
  int gcd(int a, int b);
  void normalize(int nx, int ny, int nz, float* volume);
  float calcMinSpacing(QVector3D spacing1, QVector3D spacing2);
  void calcMaxSize(vx::Array3<const float>& volume, QVector3D spacing,
                   float minSpacing, int& nx, int& ny, int& nz);
  void resampleVolume(int nx, int ny, int nz, vx::Array3<const float>& volume,
                      QVector3D spacing, float minSpacing, float* sampled);
  void initComplexArray(int nx, int ny, int nz, float* realPart,
                        std::complex<float>* complexArray);
  void calcPowerSpectrum(int nx, int ny, int nz,
                         std::complex<float>* complexArray, float* magnitude);
  void complexCorrelation(int nx, int ny, int nz, std::complex<float>* z1,
                          std::complex<float>* z2, std::complex<float>* result);
};

#endif  // VOLUMEREGISTRATION_H
