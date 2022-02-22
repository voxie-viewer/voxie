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

#ifndef ISSDETECTOR_H
#define ISSDETECTOR_H

#include "../ExtFilterIterativeClosestPoint/KdTree.hpp"

class ISSDetector {
 public:
  ISSDetector();

  std::vector<QVector3D> detectKeypoints(KdTree<QVector3D>& tree);

  /** \brief Set the radius of the spherical neighborhood used to compute the
   * scatter matrix. \param[in] salientRadius the radius of the spherical
   * neighborhood
   */
  void setSalientRadius(double salientRadius) { searchRadius_ = salientRadius; }

  /** \brief Set the radius for the application of the non maxima supression
   * algorithm. \param[in] nonMaxRadius the non maxima suppression radius
   */
  void setNonMaxRadius(double nonMaxRadius) { nonMaxRadius_ = nonMaxRadius; }

  /** \brief Set the upper bound on the ratio between the second and the first
   * eigenvalue. \param[in] gamma21 the upper bound on the ratio between the
   * second and the first eigenvalue
   */
  void setThreshold21(double gamma21) { gamma21_ = gamma21; }

  /** \brief Set the upper bound on the ratio between the third and the second
   * eigenvalue. \param[in] gamma32 the upper bound on the ratio between the
   * third and the second eigenvalue
   */
  void setThreshold32(double gamma32) { gamma32_ = gamma32; }

  /** \brief Set the minimum number of neighbors that has to be found while
   * applying the non maxima suppression algorithm. \param[in] minNeighbors the
   * minimum number of neighbors required
   */
  void setMinNeighbors(int minNeighbors) { minNeighbors_ = minNeighbors; }

 private:
  void getScatterMatrix(QVector3D& currentPoint, std::vector<QVector3D>& nn,
                        float** cov_m);

  double nonMaxRadius_ = 0;
  double searchRadius_ = 0;
  double gamma21_ = 0.6;
  double gamma32_ = 0.6;
  uint32_t minNeighbors_ = 20;

  double computeResolution(KdTree<QVector3D>& input);
};

#endif  // ISSDETECTOR_H
