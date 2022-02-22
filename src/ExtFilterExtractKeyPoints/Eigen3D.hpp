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

#ifndef EIGEN3D_H
#define EIGEN3D_H

class Eigen3D {
 public:
  Eigen3D() = delete;
  static void eigenValues3(float** a, float* real, float* imag);

 private:
  static float sign(float val) { return (0 < val) - (val < 0); };
  static void givensRotationParams(float a, float b, float& c, float& s,
                                   float& r);
  static void givensRotation(float** a, int row, int col, float** q);
  static void eigenValues2(float** a, int off, float* real, float* imag);
  static void printMatrix(float** a);
  static void houseHolderReflection(float** a, int col, float** q);
};

#endif  // EIGEN3D_H
