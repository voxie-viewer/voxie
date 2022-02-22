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

#include "Eigen3D.hpp"
#include <cmath>
#include <iostream>

void Eigen3D::printMatrix(float** a) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      std::cout << a[i][j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void Eigen3D::eigenValues2(float** a, int off, float* real, float* imag) {
  float a00 = a[off][off];
  float a01 = a[off][off + 1];
  float a10 = a[off + 1][off];
  float a11 = a[off + 1][off + 1];

  float trace = a00 + a11;
  float det = a00 * a11 - a01 * a10;
  float arg = trace * trace / 4 - det;
  real[off] = trace / 2;
  real[off + 1] = trace / 2;
  if (arg < 0) {
    imag[off] = std::sqrt(-arg);
    imag[off + 1] = -imag[off];
  } else {
    real[off] += std::sqrt(arg);
    real[off + 1] -= std::sqrt(arg);
  }
}

void Eigen3D::eigenValues3(float** a, float* real, float* imag) {
  bool isComplex = false;
  bool exchanged = false;
  float eps = 0.000000001;
  float** aTmp = new float*[3];
  float** q = new float*[3];
  float** ptrHelp = aTmp;

  for (int i = 0; i < 3; i++) {
    aTmp[i] = new float[3];
    q[i] = new float[3];
  }

  float shift = a[2][2];

  // Create Hesseberg form of A
  if (a[2][0] != 0) {
    if (a[2][1] != 0) {
      // use gauss column elemination
      float m20 = a[2][0] / a[2][1];

      // multiply with elimination matrix from RIGHT
      // M = [1  0  0]
      //     |-m 1  0|
      //     [0  0  1]
      // v[1][0] = -m20;
      a[0][0] = a[0][0] - m20 * a[0][1];
      a[1][0] = a[1][0] - m20 * a[1][1];
      a[2][0] = 0;

      // multiply with invserse elimination matrix from LEFT
      a[1][0] = m20 * a[0][0] + a[1][0];
      a[1][1] = m20 * a[0][1] + a[1][1];
      a[1][2] = m20 * a[0][2] + a[1][2];
    } else {
      // column exchange
      exchanged = true;
      for (int i = 0; i < 3; i++) {
        // apply shift before swap
        a[i][i] -= shift;
        aTmp[i][0] = a[i][0];
        a[i][0] = a[i][1];
        a[i][1] = aTmp[i][0];
      }
    }
  }
  int iteration;

  // apply shifted QR method
  for (iteration = 0; iteration < 30; iteration++) {
    float offDiag = a[2][1];

    // end iterations if value converged or small enough
    if (isComplex || std::abs(offDiag) < eps) {
      break;
    }

    // shift A
    for (int i = 0; i < 3; i++) {
      a[i][i] -= a[2][2];
    }

    // QR factorization: A = QR
    // use Givens rotation
    // R will be stored in A
    for (int i = 0; i < 2; i++) {
      givensRotation(a, i + 1, i, q);
    }

    if (exchanged) {
      // first iteration is special because need to invert column exchange
      for (int i = 0; i < 3; i++) {
        aTmp[i][0] = a[i][0];
        a[i][0] = a[i][1];
        a[i][1] = aTmp[i][0];
      }
      exchanged = false;
    }
    // multiply RQ to get next A
    // R = [x  x  x]
    //     |0  x  x|
    //     [0  0  x]
    // Q = [x  x  x]
    //     |x  x  x|
    //     [0  x  x]
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        float len = 0;
        aTmp[i][j] = 0;
        for (int k = 0; k < 3; k++) {
          len += q[k][j] * q[k][j];
          aTmp[i][j] += a[i][k] * q[k][j];
        }
        len = std::sqrt(len);
        aTmp[i][j] /= len;
      }
    }
    ptrHelp = aTmp;
    aTmp = a;
    a = ptrHelp;

    // reverse shift
    for (int i = 0; i < 2 + 1; i++) {
      a[i][i] += shift;
    }
    shift = a[2][2];

    isComplex = std::abs(offDiag - a[2][1]) < eps;
  }

  if (isComplex) {
    eigenValues2(a, 1, real, imag);
    real[0] = a[0][0];
  } else {
    eigenValues2(a, 0, real, imag);
    real[2] = a[2][2];
  }

  // sort eigenvalues
  for (int i = 0; i < 3; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (real[i] < real[j]) {
        float x = real[i];
        real[i] = real[j];
        real[j] = x;
      }
    }
  }

  if (iteration % 2 == 1) {
    aTmp = ptrHelp;
  }

  for (int i = 0; i < 3; i++) {
    delete[] aTmp[i];
    delete[] q[i];
  }
  delete[] aTmp;
  delete[] q;
}

void Eigen3D::houseHolderReflection(float** a, int col, float** q) {
  float lenSquared = 0;
  float v[3] = {};
  for (int i = col + 1; i < 3; i++) {
    lenSquared += a[i][col] * a[i][col];
  }
  for (int i = col + 1; i < 3; i++) {
    v[i] = a[i][col] / 2;
  }
  v[col + 1] = v[col + 1] + sign(a[col + 1][col]) * std::sqrt(lenSquared) / 2;
  lenSquared = 0;
  for (int i = col + 1; i < 3; i++) {
    lenSquared += v[i] * v[i];
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i < col + 1 || j < col + 1) {
        q[i][j] = 0;
      } else {
        q[i][j] = -2 * v[i] * v[j] / lenSquared;
      }
    }
    q[i][i] += 1;
  }
}

void Eigen3D::givensRotation(float** a, int row, int col, float** q) {
  float c, s, r, help;
  givensRotationParams(a[col][col], a[row][col], c, s, r);
  // assume following entry killing order -> (a[2][0]), a[1][0], a[2][1]
  a[row][col] = 0;
  a[col][col] = r;
  //  col=0, row=1  =>  j -> 1..2, i -> 0..1
  // (col=0, row=2  =>  j -> 1..2, i -> 0, 2)
  //  col=1, row=2  =>  j -> 2     i -> 1..2
  // multiply inverse Givens matrix from left with A -> Q^-1 A = R
  for (int j = col + 1; j < 3; j++) {
    help = s * a[row][j] + c * a[col][j];
    a[row][j] = c * a[row][j] - s * a[col][j];
    a[col][j] = help;
  }
  if (col == 0 && row == 1) {
    // Q10 = [c -s  0]
    //       |s  c  0|
    //       [0  0  1]
    q[0][0] = c;
    q[0][1] = -s;
    q[1][0] = s;
    q[1][1] = c;
    q[0][2] = 0;
    q[1][2] = 0;
    q[2][0] = 0;
    q[2][1] = 0;
    q[2][2] = 1;
  } else if (col == 1 && row == 2) {
    // Q21 = [1  0  0]
    //       |0  c -s|
    //       [0  s  c]
    // Q = Q10 Q21
    q[0][2] = -s * q[0][1];
    q[0][1] = c * q[0][1];
    q[1][2] = -s * q[1][1];
    q[1][1] = c * q[1][1];
    q[2][2] = c;
    q[2][1] = s;
  }
}

// stable givens rotation as in
// https://en.wikipedia.org/wiki/Givens_rotation
void Eigen3D::givensRotationParams(float a, float b, float& c, float& s,
                                   float& r) {
  if (b == 0) {
    c = sign(a);
    if (c == 0) {
      c = 1.0;
    }
    // at least in this 3d case should return identity not sign(a)
    c = 1;
    s = 0;
    r = std::abs(a);
  } else if (a == 0) {
    c = 0;
    s = sign(b);
    r = std::abs(b);
  } else if (std::abs(a) > std::abs(b)) {
    float t = b / a;
    float u = sign(a) * sqrt(1 + t * t);
    c = 1 / u;
    s = c * t;
    r = a * u;
  } else {
    float t = a / b;
    float u = sign(b) * sqrt(1 + t * t);
    s = 1 / u;
    c = s * t;
    r = b * u;
  }
}
