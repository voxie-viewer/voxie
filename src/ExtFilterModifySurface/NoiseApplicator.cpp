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

#include "NoiseApplicator.hpp"

#include <QtCore/QDebug>

#include <random>

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

NoiseApplicator::NoiseApplicator() {}

void NoiseApplicator::addNoise(vx::Array2<float>& vertices,
                               vx::Array2<const uint32_t> triangles,
                               double level) {
  calculateNormals(vertices, triangles);
  double deviation = level * meanLength;
  qDebug() << deviation;

  std::default_random_engine generator;
  std::normal_distribution<double> distribution(0, deviation);

  for (size_t i = 0; i < vertices.size<0>(); i++) {
    double length = distribution(generator);
    vertices(i, 0) = vertices(i, 0) + normals[i].x() * length;
    vertices(i, 1) = vertices(i, 1) + normals[i].y() * length;
    vertices(i, 2) = vertices(i, 2) + normals[i].z() * length;
  }
}

void NoiseApplicator::calculateNormals(vx::Array2<float>& vertices,
                                       vx::Array2<const uint32_t> triangles) {
  meanLength = 0;
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    normals.append(QVector3D(0, 0, 0));
  }

  for (size_t i = 0; i < triangles.size<0>(); i++) {
    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));

    QVector3D direction = first - second;
    direction.normalize();
    float area = (first - second).length() *
                 third.distanceToLine(first, direction) / 2.0;
    QVector3D normal =
        QVector3D().normal(first * 500, second * 500, third * 500);

    meanLength += (first - second).length();
    meanLength += (first - third).length();
    meanLength += (second - third).length();

    for (int j = 0; j < 3; j++) {
      normals[triangles(i, j)] += normal * area;
    }
  }
  meanLength = meanLength / (3 * triangles.size<0>());
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    normals[i] = normals.at(i) * 5000;
    normals[i].normalize();
  }
}
