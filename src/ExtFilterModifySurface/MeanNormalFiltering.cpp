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

#include "MeanNormalFiltering.hpp"

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

MeanNormalFiltering::MeanNormalFiltering() {
  QList<QVector3D> normals;
  QList<float> areas;
  QList<QVector3D> centroids;
  QList<QVector3D> m;
  QList<QVector3D> normalsPerVertex;
  QList<float> neighbouringAreasPerVertex;
  QList<QVector3D> v;
  QList<int> partOf;
}

void MeanNormalFiltering::compute(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles,
    int iterations,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  for (int i = 0; i < iterations; i++) {
    normals.clear();
    areas.clear();
    centroids.clear();
    m.clear();
    weightedNormalsPerVertex.clear();
    neighbouringAreasPerVertex.clear();
    v.clear();
    partOf.clear();
    calculateTriangleInfo(vertices, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (3.0 * i + 1) / (3.0 * iterations), vx::emptyOptions()));
    executeStepOneTwo(vertices, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (3.0 * i + 2) / (3.0 * iterations), vx::emptyOptions()));
    executeStepThree(vertices, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (3.0 * i + 3) / (3.0 * iterations), vx::emptyOptions()));
  }
}

void MeanNormalFiltering::calculateTriangleInfo(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    neighbouringAreasPerVertex.append(0.0);
    weightedNormalsPerVertex.append(QVector3D());
    v.append(QVector3D());
    partOf.append(0);
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
    QVector3D normal = QVector3D().normal(
        first * 500, second * 500, third * 500);  // TODO: bessere Lösung hier
    normals.append(normal);
    areas.append(area);
    centroids.append(QVector3D((first.x() + second.x() + third.x()) / 3,
                               (first.y() + second.y() + third.y()) / 3,
                               (first.z() + second.z() + third.z()) / 3));
    m.append(QVector3D());
    for (int j = 0; j < 3; j++) {
      neighbouringAreasPerVertex[triangles(i, j)] += area;
      weightedNormalsPerVertex[triangles(i, j)] += normal * area;
      partOf[triangles(i, j)]++;
    }
  }
}

void MeanNormalFiltering::executeStepOneTwo(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(vertices);
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    float neighbouringArea = 0.0;
    // Step One
    for (int j = 0; j < 3; j++) {
      float areaWithoutThis =
          neighbouringAreasPerVertex.at(triangles(i, j)) - areas.at(i);
      QVector3D normalsWithoutThis =
          weightedNormalsPerVertex.at(triangles(i, j)) -
          normals.at(i) * areas.at(i);
      m[i] += normalsWithoutThis;
      neighbouringArea += areaWithoutThis;
    }
    m[i] /= neighbouringArea;
    // Step Two
    m[i].normalize();
  }
}

void MeanNormalFiltering::executeStepThree(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    for (size_t j = 0; j < 3; j++) {
      QVector3D p(vertices(triangles(i, j), 0), vertices(triangles(i, j), 1),
                  vertices(triangles(i, j), 2));
      v[triangles(i, j)] +=
          areas.at(i) *
          (QVector3D().dotProduct((centroids[i] - p), m.at(i)) * m.at(i));
    }
  }
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    vertices(i, 0) += v.at(i).x() / neighbouringAreasPerVertex.at(i);
    vertices(i, 1) += v.at(i).y() / neighbouringAreasPerVertex.at(i);
    vertices(i, 2) += v.at(i).z() / neighbouringAreasPerVertex.at(i);
  }
}
