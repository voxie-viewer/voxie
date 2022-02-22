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

#include "VolumeRegistration.hpp"

#include <QtGui/QVector3D>

#include <cmath>

#if USE_FFTW
#include <fftw3.h>
#else
#include <kiss_fftnd.h>
#endif

#define REAL 0
#define IMAG 1
#define PI 3.14159265
#define EPSILON 0.000001
#define ANGLE_STEPS 720

VolumeRegistration::VolumeRegistration() {}

void VolumeRegistration::compute(
    vx::Array3<const float>& vxRefVolume, vx::Array3<const float>& vxInVolume,
    QVector3D spacing1, QVector3D spacing2, QVector3D origin1,
    QVector3D origin2, std::tuple<double, double, double>& translation,
    std::tuple<double, double, double, double>& rotation,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  float minSpacing = calcMinSpacing(spacing1, spacing2);

  int nx = 0, ny = 0, nz = 0;
  // nx = ny = nz = max
  calcMaxSize(vxRefVolume, spacing1, minSpacing, nx, ny, nz);
  calcMaxSize(vxInVolume, spacing2, minSpacing, nx, ny, nz);

  float* refVolume = new float[nx * ny * nz];
  float* inVolume = new float[nx * ny * nz];
  resampleVolume(nx, ny, nz, vxRefVolume, spacing1, minSpacing, refVolume);
  resampleVolume(nx, ny, nz, vxInVolume, spacing2, minSpacing, inVolume);
  normalize(nx, ny, nz, refVolume);
  normalize(nx, ny, nz, inVolume);

#if USE_FFTW
  std::complex<float>* inComplex1 = (std::complex<float>*)fftwf_malloc(
      sizeof(std::complex<float>) * nx * ny * nz);
  std::complex<float>* inComplex2 = (std::complex<float>*)fftwf_malloc(
      sizeof(std::complex<float>) * nx * ny * nz);
  std::complex<float>* outComplex1 = (std::complex<float>*)fftwf_malloc(
      sizeof(std::complex<float>) * nx * ny * nz);
  std::complex<float>* outComplex2 = (std::complex<float>*)fftwf_malloc(
      sizeof(std::complex<float>) * nx * ny * nz);
  fftwf_plan plan1 = fftwf_plan_dft_3d(nx, ny, nz, (fftwf_complex*)inComplex1,
                                       (fftwf_complex*)outComplex1,
                                       FFTW_FORWARD, FFTW_ESTIMATE);
  fftwf_plan plan2 = fftwf_plan_dft_3d(nx, ny, nz, (fftwf_complex*)inComplex2,
                                       (fftwf_complex*)outComplex2,
                                       FFTW_FORWARD, FFTW_ESTIMATE);
#else
  std::complex<float>* inComplex1 = new std::complex<float>[nx * ny * nz];
  std::complex<float>* inComplex2 = new std::complex<float>[nx * ny * nz];
  std::complex<float>* outComplex1 = new std::complex<float>[nx * ny * nz];
  std::complex<float>* outComplex2 = new std::complex<float>[nx * ny * nz];
  int dims[3] = {nx, ny, nz};
  auto plan = kiss_fftnd_alloc(dims, 3, false, nullptr, nullptr);
#endif

  initComplexArray(nx, ny, nz, refVolume, inComplex1);
  initComplexArray(nx, ny, nz, inVolume, inComplex2);

#if USE_FFTW
  fftwf_execute(plan1);
  fftwf_execute(plan2);
#else
  kiss_fftnd(plan, (kiss_fft_cpx*)inComplex1, (kiss_fft_cpx*)outComplex1);
  kiss_fftnd(plan, (kiss_fft_cpx*)inComplex2, (kiss_fft_cpx*)outComplex2);
#endif
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.10, vx::emptyOptions()));

  // reuse storage
  float* inP = (float*)inComplex1;
  float* refMagnitude = inP;
  float* inMagnitude = &(inP[nz * ny * nx]);
  calcPowerSpectrum(nx, ny, nz, outComplex1, refMagnitude);
  calcPowerSpectrum(nx, ny, nz, outComplex2, inMagnitude);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.15, vx::emptyOptions()));

  periodicShift(nx / 2, ny / 2, nz / 2, nx, ny, nz, refMagnitude);
  periodicShift(nx / 2, ny / 2, nz / 2, nx, ny, nz, inMagnitude);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.20, vx::emptyOptions()));

  std::vector<double> rotationAxis;
  rotationAxis = findRotationAxis(nx, ny, nz, refMagnitude, inMagnitude);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.25, vx::emptyOptions()));

  // find angle, either angle or angle + PI (indistinguishable due to symmetry)
  float angle =
      findRotationAngle(nx, ny, nz, rotationAxis, refMagnitude, inMagnitude);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.30, vx::emptyOptions()));

  std::vector<double> center1 = {-origin1.x() / minSpacing,
                                 -origin1.y() / minSpacing,
                                 -origin1.z() / minSpacing};
  std::vector<double> center2 = {-origin2.x() / minSpacing,
                                 -origin2.y() / minSpacing,
                                 -origin2.z() / minSpacing};

  float maxValue = 0;
  float angles[] = {angle, (float)(angle + PI)};
  int maxDx = 0, maxDy = 0, maxDz = 0, maxI = 0;
  float* rotatedVolume;

  // calculate cross correlation for both angles and take the best angle and
  // shift
  for (int iAngle = 0; iAngle < 2; iAngle++) {
    rotatedVolume = &(inP[iAngle * nx * ny * nz]);
    rotateVolume(nx, ny, nz, center1, center2, rotationAxis, -angles[iAngle],
                 inVolume, rotatedVolume);
    HANDLEDBUSPENDINGREPLY(
        prog.opGen().SetProgress(0.40 + iAngle * 0.30, vx::emptyOptions()));

    initComplexArray(nx, ny, nz, rotatedVolume, inComplex2);
    HANDLEDBUSPENDINGREPLY(
        prog.opGen().SetProgress(0.45 + iAngle * 0.30, vx::emptyOptions()));

#if USE_FFTW
    fftwf_execute(plan2);
#else
    kiss_fftnd(plan, (kiss_fft_cpx*)inComplex2, (kiss_fft_cpx*)outComplex2);
#endif
    HANDLEDBUSPENDINGREPLY(
        prog.opGen().SetProgress(0.50 + iAngle * 0.30, vx::emptyOptions()));

    outComplex1[0] = std::complex<float>(1, outComplex1[0].imag());
    outComplex2[0] = std::complex<float>(1, outComplex2[0].imag());
    complexCorrelation(nx, ny, nz, outComplex1, outComplex2, inComplex2);
    HANDLEDBUSPENDINGREPLY(
        prog.opGen().SetProgress(0.55 + iAngle * 0.30, vx::emptyOptions()));

#if USE_FFTW
    fftwf_execute(plan2);
#else
    kiss_fftnd(plan, (kiss_fft_cpx*)inComplex2, (kiss_fft_cpx*)outComplex2);
#endif
    HANDLEDBUSPENDINGREPLY(
        prog.opGen().SetProgress(0.60 + iAngle * 0.30, vx::emptyOptions()));

    // find location with maximum cross correlation
    for (int z = 0; z < nz; z++) {
      for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
          int i = x + y * nx + z * nx * ny;
          if (outComplex2[i].real() > maxValue) {
            maxValue = outComplex2[i].real();
            maxDx = x;
            maxDy = y;
            maxDz = z;
            maxI = iAngle;
          }
        }
      }
    }
    HANDLEDBUSPENDINGREPLY(
        prog.opGen().SetProgress(0.65 + iAngle * 0.30, vx::emptyOptions()));
  }
  rotatedVolume = &(inP[maxI * nx * ny * nz]);
  std::vector<int> dVec =
      findBestShift(nx, ny, nz, refVolume, rotatedVolume, maxDx, maxDy, maxDz);

  std::cout << "angle: " << angles[maxI] << std::endl;
  std::cout << "dx: " << dVec.at(0) << ", dy: " << dVec.at(1)
            << ", dz: " << dVec.at(2) << std::endl;

  // write results
  double ca = std::cos(angles[maxI] / 2);
  double sa = std::sin(angles[maxI] / 2);
  rotation = std::tuple<double, double, double, double>{
      ca, rotationAxis.at(0) * sa, rotationAxis.at(1) * sa,
      rotationAxis.at(2) * sa};
  translation = std::tuple<double, double, double>{dVec.at(0) * minSpacing,
                                                   dVec.at(1) * minSpacing,
                                                   dVec.at(2) * minSpacing};

#if USE_FFTW
  fftwf_destroy_plan(plan1);
  fftwf_destroy_plan(plan2);
  fftwf_free(inComplex1);
  fftwf_free(inComplex2);
  fftwf_free(outComplex1);
  fftwf_free(outComplex2);
  fftwf_cleanup();
#else
  kiss_fft_free(plan);
  delete[] inComplex1;
  delete[] inComplex2;
  delete[] outComplex1;
  delete[] outComplex2;
  kiss_fft_cleanup();
#endif
  delete[] refVolume;
  delete[] inVolume;

  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(1.00, vx::emptyOptions()));
}

std::vector<int> VolumeRegistration::findBestShift(int nx, int ny, int nz,
                                                   float* inVolume, float* help,
                                                   int dx, int dy, int dz) {
  double correlation[8] = {0};

  // calculated shifts dx, dy, dz are always positive
  for (int z = 0; z < nz; z++) {
    int zL = z - ((dz - nz) % nz);
    int zR = z - dz;
    for (int y = 0; y < ny; y++) {
      int yL = y - ((dy - ny) % ny);
      int yR = y - dy;
      for (int x = 0; x < nx; x++) {
        int xL = x - ((dx - nx) % nx);
        int xR = x - dx;
        int i = x + y * nx + z * nx * ny;
        if (xL < nx && yL < ny && zL < nz) {
          correlation[0] += help[i] * inVolume[xL + yL * nx + zL * nx * nz];
        }
        if (xL < nx && yL < ny && zR >= 0) {
          correlation[1] += help[i] * inVolume[xL + yL * nx + zR * nx * ny];
        }
        if (xL < nx && yR >= 0 && zL < nz) {
          correlation[2] += help[i] * inVolume[xL + yR * nx + zL * nx * ny];
        }
        if (xL < nx && yR >= 0 && zR >= 0) {
          correlation[3] += help[i] * inVolume[xL + yR * nx + zR * nx * ny];
        }
        if (xR >= 0 && yL < ny && zL < nz) {
          correlation[4] += help[i] * inVolume[xR + yL * nx + zL * nx * ny];
        }
        if (xR >= 0 && yL < ny && zR >= 0) {
          correlation[5] += help[i] * inVolume[xR + yL * nx + zR * nx * ny];
        }
        if (xR >= 0 && yR >= 0 && zL < nz) {
          correlation[6] += help[i] * inVolume[xR + yR * nx + zL * nx * ny];
        }
        if (xR >= 0 && yR >= 0 && zR >= 0) {
          correlation[7] += help[i] * inVolume[xR + yR * nx + zR * nx * ny];
        }
      }
    }
  }

  double maxCorrelation = correlation[0];
  int maxI = 0;
  for (int i = 0; i < 8; i++) {
    if (correlation[i] > maxCorrelation) {
      maxCorrelation = correlation[i];
      maxI = i;
    }
  }

  if ((maxI & 4) == 0) {
    dx = (dx - nx) % nx;
  }
  if ((maxI & 2) == 0) {
    dy = (dy - ny) % ny;
  }
  if ((maxI & 1) == 0) {
    dz = (dz - nz) % nz;
  }

  return {-dx, -dy, -dz};
}

void VolumeRegistration::rotateVolume(int nx, int ny, int nz,
                                      std::vector<double> center1,
                                      std::vector<double> center2,
                                      std::vector<double> rotationAxis,
                                      float angle, float* inVolume,
                                      float* outVolume) {
  float axisX = rotationAxis.at(0);
  float axisY = rotationAxis.at(1);
  float axisZ = rotationAxis.at(2);

  float matrix[9];
  fillRotationMatrix(axisX, axisY, axisZ, angle, matrix);
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        // shift center of rotation
        float x1 = x - center1.at(0);
        float y1 = y - center1.at(1);
        float z1 = z - center1.at(2);
        multMatrix(matrix, x1, y1, z1);
        x1 += center2.at(0);
        y1 += center2.at(1);
        z1 += center2.at(2);
        outVolume[x + y * nx + z * nx * ny] =
            sampleVolume(x1, y1, z1, nx, ny, nz, inVolume);
      }
    }
  }
}

std::vector<double> VolumeRegistration::findRotationAxis(int nx, int ny, int nz,
                                                         float* volume1,
                                                         float* volume2) {
  int maxRadius = std::min(nx, std::min(ny, nz)) / 2;
  int* lineVotes = new int[ANGLE_STEPS * ANGLE_STEPS];

  float pi_steps = PI / ANGLE_STEPS;
  for (int phiStep = 0; phiStep < ANGLE_STEPS; phiStep++) {
    float sinPhi = std::sin(pi_steps * phiStep);
    float cosPhi = std::cos(pi_steps * phiStep);
    for (int tetaStep = 0; tetaStep < ANGLE_STEPS; tetaStep++) {
      int i = tetaStep + phiStep * ANGLE_STEPS;
      lineVotes[i] = 0;
      float sinTeta = std::sin(pi_steps * tetaStep);
      float cosTeta = std::cos(pi_steps * tetaStep);
      for (int r = 5; r < maxRadius; r++) {
        // get x,y,z
        float x = nx / 2 + r * sinTeta * cosPhi;
        float y = ny / 2 + r * sinTeta * sinPhi;
        float z = nz / 2 + r * cosTeta;

        // accumulate sum
        float value1 = sampleVolume(x, y, z, nx, ny, nz, volume1);
        float value2 = sampleVolume(x, y, z, nx, ny, nz, volume2);
        float valueI = abs((value1 - value2) / (value1 + value2 + EPSILON));
        if (valueI < 0.002) {
          lineVotes[i]++;
        }
      }
    }
  }

  // find max
  int max = lineVotes[0];
  double teta = 0;
  double phi = 0;
  for (int phiStep = 0; phiStep < ANGLE_STEPS; phiStep++) {
    for (int tetaStep = 0; tetaStep < ANGLE_STEPS; tetaStep++) {
      int i = tetaStep + phiStep * ANGLE_STEPS;
      if (lineVotes[i] >= max) {
        max = lineVotes[i];
        teta = pi_steps * tetaStep;
        phi = pi_steps * phiStep;
      }
    }
  }

  // DEBUGGING
  int count = 0;
  for (int tetaStep = 0; tetaStep < ANGLE_STEPS; tetaStep++) {
    for (int phiStep = 0; phiStep < ANGLE_STEPS; phiStep++) {
      int i = tetaStep + phiStep * ANGLE_STEPS;
      if (lineVotes[i] > (max / 1.2)) {
        int localMaxCount = 0;
        if (lineVotes[((tetaStep - 1 + ANGLE_STEPS) % ANGLE_STEPS) +
                      phiStep * ANGLE_STEPS] <= lineVotes[i]) {
          if (lineVotes[((tetaStep - 1 + ANGLE_STEPS) % ANGLE_STEPS) +
                        phiStep * ANGLE_STEPS] < lineVotes[i]) {
            localMaxCount++;
          }
          if (lineVotes[((tetaStep + 1 + ANGLE_STEPS) % ANGLE_STEPS) +
                        phiStep * ANGLE_STEPS] <= lineVotes[i]) {
            if (lineVotes[((tetaStep + 1 + ANGLE_STEPS) % ANGLE_STEPS) +
                          phiStep * ANGLE_STEPS] < lineVotes[i]) {
              localMaxCount++;
            }
            if (lineVotes[tetaStep +
                          ((phiStep - 1 + ANGLE_STEPS) % ANGLE_STEPS) *
                              ANGLE_STEPS] <= lineVotes[i]) {
              if (lineVotes[tetaStep +
                            ((phiStep - 1 + ANGLE_STEPS) % ANGLE_STEPS) *
                                ANGLE_STEPS] < lineVotes[i]) {
                localMaxCount++;
              }
              if (lineVotes[tetaStep +
                            ((phiStep + 1 + ANGLE_STEPS) % ANGLE_STEPS) *
                                ANGLE_STEPS] <= lineVotes[i]) {
                if (lineVotes[tetaStep +
                              ((phiStep + 1 + ANGLE_STEPS) % ANGLE_STEPS) *
                                  ANGLE_STEPS] < lineVotes[i]) {
                  localMaxCount++;
                }
                if (localMaxCount > -1) {
                  count++;
                  std::cout << (pi_steps * tetaStep) << ", "
                            << (pi_steps * phiStep) << ", " << lineVotes[i]
                            << std::endl;
                }
              }
            }
          }
        }
      }
    }
  }
  std::cout << "count: " << count << std::endl;
  // DEBUGGING

  std::cout << "teta: " << teta << ", phi: " << phi << std::endl;

  std::vector<double> rotationAxis = {std::sin(teta) * std::cos(phi),
                                      std::sin(teta) * std::sin(phi),
                                      std::cos(teta)};

  delete[] lineVotes;

  return rotationAxis;
}

float VolumeRegistration::findRotationAngle(int nx, int ny, int nz,
                                            std::vector<double> rotationAxis,
                                            float* vol1, float* vol2) {
  int maxRadius = std::min(nx, std::min(ny, nz)) / 2;
  int minRadius = 5;
  float* lineSum1 = new float[ANGLE_STEPS * (maxRadius - minRadius)];
  float* lineSum2 = new float[ANGLE_STEPS * (maxRadius - minRadius)];

  float axisX = rotationAxis.at(0);
  float axisY = rotationAxis.at(1);
  float axisZ = rotationAxis.at(2);

  float xStart = 0, yStart = 0, zStart = 0;
  if (axisX == 0) {
    xStart = 1;
  } else if (axisY == 0) {
    yStart = 1;
  } else if (axisZ == 0) {
    zStart = 1;
  } else {
    float len = std::sqrt(axisX * axisX + axisY * axisY);
    xStart = -axisY / len;
    yStart = axisX / len;
    zStart = 0;
  }

  float matrix[9];
  for (int psiStep = 0; psiStep < ANGLE_STEPS; psiStep++) {
    float angle = PI * psiStep / ANGLE_STEPS;
    fillRotationMatrix(axisX, axisY, axisZ, angle, matrix);
    for (int r = minRadius; r < maxRadius; r++) {
      // get x,y,z
      float x = xStart;
      float y = yStart;
      float z = zStart;
      multMatrix(matrix, x, y, z);
      x = nx / 2 + r * x;
      y = ny / 2 + r * y;
      z = nz / 2 + r * z;

      // accumulate sum
      lineSum1[psiStep + (r - minRadius) * ANGLE_STEPS] =
          sampleVolume(x, y, z, nx, ny, nz, vol1);
      lineSum2[psiStep + (r - minRadius) * ANGLE_STEPS] =
          sampleVolume(x, y, z, nx, ny, nz, vol2);
    }
  }

  float maxValue = 0;
  float angle = 0;
  for (int shift = 0; shift < ANGLE_STEPS; shift++) {
    float value = 0;
    for (int r = minRadius; r < maxRadius; r++) {
      for (int psiStep = 0; psiStep < ANGLE_STEPS; psiStep++) {
        value += lineSum1[((psiStep + shift) % ANGLE_STEPS) +
                          (r - minRadius) * ANGLE_STEPS] *
                 lineSum2[psiStep + (r - minRadius) * ANGLE_STEPS];
      }
    }
    if (value > maxValue) {
      maxValue = value;
      angle = PI * shift / ANGLE_STEPS;
    }
  }

  delete[] lineSum1;
  delete[] lineSum2;

  return angle;
}

int VolumeRegistration::gcd(int a, int b) {
  while (b != 0) {
    int tmp = b;
    b = a % b;
    a = tmp;
  }
  return a;
}

template <typename T>
void VolumeRegistration::periodicShift3(int dx, int dy, int dz, int nx, int ny,
                                        int nz, T* data) {
  dx = -dx;
  dy = -dy;
  dz = -dz;
  if (dx <= 0) {
    dx += nx;
  }
  if (dy <= 0) {
    dy += ny;
  }
  if (dz <= 0) {
    dz += nz;
  }
  int gcdX = gcd(nx, dx);
  int gcdY = gcd(ny, dy);
  int gcdZ = gcd(nz, dz);
  int n1 = nx / gcdX;
  int n2 = ny / gcdY;
  int n3 = nz / gcdZ;

  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < gcdX; x++) {
        int x1 = x;
        int yz = y * nx + z * nx * ny;
        int i = x + yz;
        T tmp = data[i];
        int j = i;
        for (int k = 1; k < n1; k++) {
          x1 += dx;
          if (x1 >= nx) {
            x1 -= nx;
          }
          j = x1 + yz;
          data[i] = data[j];
          i = j;
        }
        data[j] = tmp;
      }
    }
  }

  for (int z = 0; z < nz; z++) {
    for (int x = 0; x < nx; x++) {
      for (int y = 0; y < gcdY; y++) {
        int y1 = y;
        int xz = x + z * nx * ny;
        int i = y1 * nx + xz;
        T tmp = data[i];
        int j = i;
        for (int k = 1; k < n2; k++) {
          y1 += dy;
          if (y1 >= ny) {
            y1 -= ny;
          }
          j = y1 * nx + xz;
          data[i] = data[j];
          i = j;
        }
        data[j] = tmp;
      }
    }
  }

  for (int y = 0; y < ny; y++) {
    for (int x = 0; x < nx; x++) {
      for (int z = 0; z < gcdZ; z++) {
        int z1 = z;
        int xy = x + y * nx;
        int i = z * nx * ny + xy;
        T tmp = data[i];
        int j = i;
        for (int k = 1; k < n3; k++) {
          z1 += dz;
          if (z1 >= nz) {
            z1 -= nz;
          }
          j = z1 * nx * ny + xy;
          data[i] = data[j];
          i = j;
        }
        data[j] = tmp;
      }
    }
  }
}

template <typename T>
void VolumeRegistration::periodicShift2(int dx, int dy, int dz, int nx, int ny,
                                        int nz, T* data) {
  dx = -dx;
  dy = -dy;
  dz = -dz;
  if (dx <= 0) {
    dx += nx;
  }
  if (dy <= 0) {
    dy += ny;
  }
  if (dz <= 0) {
    dz += nz;
  }
  int gcdX = gcd(nx, dx);
  int gcdY = gcd(ny, dy);
  int gcdZ = gcd(nz, dz);
  int n1 = nx / gcdX;
  int n2 = ny / gcdY;
  int n3 = nz / gcdZ;
  int gcdXY = gcd(n1, n2);
  int gcdXZ = gcd(n1, n3);
  int gcdYZ = gcd(n2, n3);
  int gcdXYZ = gcd(gcdXY, gcdXZ);

  int numShifts = (n1 * n2 / gcdXY);
  numShifts = numShifts * n3 / gcd(numShifts, n3);

  for (int z = 0; z < gcdZ; z++) {
    for (int y = 0; y < gcdY * gcdYZ; y++) {
      for (int x = 0; x < gcdX * gcdXY * gcdXZ / gcdXYZ; x++) {
        int z1 = z;
        int y1 = y;
        int x1 = x;
        int i = x + y * nx + z * nx * ny;
        T tmp = data[i];
        int j = i;
        for (long k = 1; k < numShifts; k++) {
          x1 = (x1 + dx);
          if (x1 >= nx) {
            x1 -= nx;
          }
          y1 = (y1 + dy);
          if (y1 >= ny) {
            y1 -= ny;
          }
          z1 = (z1 + dz);
          if (z1 >= nz) {
            z1 -= nz;
          }
          j = x1 + y1 * nx + z1 * nx * ny;
          data[i] = data[j];
          i = j;
        }
        data[j] = tmp;
      }
    }
  }
}

template <typename T>
void VolumeRegistration::periodicShift(int dx, int dy, int dz, int nx, int ny,
                                       int nz, T* data) {
  T* help = new T[nx * ny * nz];
  if (dx <= 0) {
    dx += nx;
  }
  if (dy <= 0) {
    dy += ny;
  }
  if (dz <= 0) {
    dz += nz;
  }
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        int j =
            (x + dx) % nx + ((y + dy) % ny) * nx + ((z + dz) % nz) * nx * ny;
        help[j] = data[i];
      }
    }
  }
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        data[i] = help[i];
      }
    }
  }
  delete[] help;
}

void VolumeRegistration::fillRotationMatrix(float axisX, float axisY,
                                            float axisZ, float angle,
                                            float* matrix) {
  float cos = std::cos(angle);
  float sin = std::sin(angle);

  matrix[0] = (axisX * axisX * (1 - cos) + cos);
  matrix[1] = (axisY * axisX * (1 - cos) - axisZ * sin);
  matrix[2] = (axisZ * axisX * (1 - cos) + axisY * sin);
  matrix[3] = (axisX * axisY * (1 - cos) + axisZ * sin);
  matrix[4] = (axisY * axisY * (1 - cos) + cos);
  matrix[5] = (axisZ * axisY * (1 - cos) - axisX * sin);
  matrix[6] = (axisX * axisZ * (1 - cos) - axisY * sin);
  matrix[7] = (axisY * axisZ * (1 - cos) + axisX * sin);
  matrix[8] = (axisZ * axisZ * (1 - cos) + cos);
}

void VolumeRegistration::multMatrix(float* matrix, float& x, float& y,
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

float VolumeRegistration::sampleVolume(float x, float y, float z, int nx,
                                       int ny, int nz, const float* volume) {
  if (x < 0 || x > nx - 1) {
    return 0;
  }
  if (y < 0 || y > ny - 1) {
    return 0;
  }
  if (z < 0 || z > nz - 1) {
    return 0;
  }
  int x1 = std::floor(x);
  int x2 = std::ceil(x);
  int y1 = std::floor(y);
  int y2 = std::ceil(y);
  int z1 = std::floor(z);
  int z2 = std::ceil(z);
  float interpolateXY1 =
      volume[getIndex(x1, y1, z1, nx, ny)] * (1 - (x - x1)) * (1 - (y - y1)) +
      volume[getIndex(x2, y1, z1, nx, ny)] * (x - x1) * (1 - (y - y1)) +
      volume[getIndex(x1, y2, z1, nx, ny)] * (1 - (x - x1)) * (y - y1) +
      volume[getIndex(x2, y2, z1, nx, ny)] * (x - x1) * (y - y1);
  float interpolateXY2 =
      volume[getIndex(x1, y1, z2, nx, ny)] * (1 - (x - x1)) * (1 - (y - y1)) +
      volume[getIndex(x2, y1, z2, nx, ny)] * (x - x1) * (1 - (y - y1)) +
      volume[getIndex(x1, y2, z2, nx, ny)] * (1 - (x - x1)) * (y - y1) +
      volume[getIndex(x2, y2, z2, nx, ny)] * (x - x1) * (y - y1);
  return interpolateXY1 * (1 - (z - z1)) + interpolateXY2 * (z - z1);
}

int VolumeRegistration::getIndex(int x, int y, int z, int nx, int ny) {
  return x + y * nx + z * nx * ny;
}

float VolumeRegistration::calcMinSpacing(QVector3D spacing1,
                                         QVector3D spacing2) {
  float minSpacing = spacing1[0];
  for (int i = 0; i < 3; i++) {
    if (spacing1[i] < minSpacing) {
      minSpacing = spacing1[i];
    }
    if (spacing2[i] < minSpacing) {
      minSpacing = spacing2[i];
    }
  }
  return minSpacing;
}

void VolumeRegistration::calcMaxSize(vx::Array3<const float>& volume,
                                     QVector3D spacing, float minSpacing,
                                     int& nx, int& ny, int& nz) {
  int nx1 = volume.size<0>();
  int ny1 = volume.size<1>();
  int nz1 = volume.size<2>();

  int maxSize[3];
  maxSize[0] = std::max((int)std::round(nx1 * spacing.x() / minSpacing), nx);
  maxSize[1] = std::max((int)std::round(ny1 * spacing.y() / minSpacing), ny);
  maxSize[2] = std::max((int)std::round(nz1 * spacing.z() / minSpacing), nz);
  int max = maxSize[0];
  for (int i = 1; i < 3; i++) {
    if (maxSize[i] > max) {
      max = maxSize[i];
    }
  }
  int padding = 0;
  int primes[4] = {2, 3, 5, 7};
  // power of 2 3 5 7 works best
  if ((max & 1) == 1) {
    max++;
  }
  int current = max;
  // (brute force) prime factorization, only consider numbers divisible by 2
  // doesn't need to be optimal best solution
  // still fast if maximum size is not much larger than a few thousand
  while (current != 1) {
    bool found = false;
    for (int i = 0; i < 4; i++) {
      int q = current / primes[i];
      while (q * primes[i] == current) {
        current = q;
        found = true;
        q = current / primes[i];
      }
    }
    if (!found) {
      padding += 2;
      current = max + padding;
    }
    found = false;
  }
  max = max + padding;
  nx = max;
  ny = max;
  nz = max;
}

void VolumeRegistration::resampleVolume(int nx, int ny, int nz,
                                        vx::Array3<const float>& volume,
                                        QVector3D spacing, float minSpacing,
                                        float* sampled) {
  int nx1 = volume.size<0>();
  int ny1 = volume.size<1>();
  int nz1 = volume.size<2>();
  for (int z = 0; z < nz; z++) {
    float z1 = z / (spacing.z() / minSpacing);
    for (int y = 0; y < ny; y++) {
      float y1 = y / (spacing.y() / minSpacing);
      for (int x = 0; x < nx; x++) {
        float x1 = x / (spacing.x() / minSpacing);
        sampled[x + y * nx + z * nx * ny] =
            sampleVolume(x1, y1, z1, nx1, ny1, nz1, volume.data());
      }
    }
  }
}

void VolumeRegistration::initComplexArray(int nx, int ny, int nz,
                                          float* realPart,
                                          std::complex<float>* complexArray) {
  // initialize
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        complexArray[i] = std::complex<float>(realPart[i], 0);
      }
    }
  }
}

void VolumeRegistration::calcPowerSpectrum(int nx, int ny, int nz,
                                           std::complex<float>* complexArray,
                                           float* magnitude) {
  // calculate normalized differences of maginitudes
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        magnitude[i] =
            log(1 + (complexArray[i].real() * complexArray[i].real() +
                     complexArray[i].imag() * complexArray[i].imag()));
        if (std::isinf(magnitude[i])) {
          std::cout << "fft1 => inf";
        }
      }
    }
  }
}

void VolumeRegistration::complexCorrelation(int nx, int ny, int nz,
                                            std::complex<float>* z1,
                                            std::complex<float>* z2,
                                            std::complex<float>* result) {
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        // complex multiplication
        float a = z1[i].real();
        float b = -z1[i].imag();  // complex conjugate
        float c = z2[i].real();
        float d = z2[i].imag();
        result[i] = std::complex<float>(
            (a * c - b * d) / (nx * ny * nz),
            -(a * d + b * c) /
                (nx * ny * nz));  // minus to perform ifft with fft
      }
    }
  }
}

void VolumeRegistration::normalize(int nx, int ny, int nz, float* volume) {
  float max = volume[0];
  float min = volume[0];
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        if (volume[i] > max) {
          max = volume[i];
        }
        if (volume[i] < min) {
          min = volume[i];
        }
      }
    }
  }
  for (int z = 0; z < nz; z++) {
    for (int y = 0; y < ny; y++) {
      for (int x = 0; x < nx; x++) {
        int i = x + y * nx + z * nx * ny;
        volume[i] = (volume[i] - min) / (max - min) * 256;
      }
    }
  }
}
