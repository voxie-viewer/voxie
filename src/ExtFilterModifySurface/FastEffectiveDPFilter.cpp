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

#include "FastEffectiveDPFilter.hpp"

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

FastEffectiveDPFilter::FastEffectiveDPFilter() {
  threshold = 0;
  QList<QVector3D> filteredNormals;
  QList<QVector3D> normals;
  QList<QList<int>> neighbours;
  QList<QVector3D> differences;
  QList<QVector3D> centroids;
}

void FastEffectiveDPFilter::compute(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles,
    double threshold, int iterations,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  this->threshold = threshold;
  for (int i = 0; i < iterations; i++) {
    filteredNormals.clear();
    normals.clear();
    neighbours.clear();
    differences.clear();
    centroids.clear();

    getTrianglesInfo(vertices, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (double)(i + 0.2) / (double)iterations, vx::emptyOptions()));
    setUpNeighbours(vertices, triangles);
    filterNormals(vertices, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (double)(i + 0.8) / (double)iterations, vx::emptyOptions()));
    updateVertices(vertices, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (double)(i + 1) / (double)iterations, vx::emptyOptions()));
  }
}

void FastEffectiveDPFilter::getTrianglesInfo(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));

    centroids.append(QVector3D((first.x() + second.x() + third.x()) / 3,
                               (first.y() + second.y() + third.y()) / 3,
                               (first.z() + second.z() + third.z()) / 3));

    QVector3D normal = QVector3D().normal(
        first * 500, second * 500, third * 500);  // TODO: bessere Lösung hier
    normals.append(normal);
    filteredNormals.append(QVector3D());
  }
}

void FastEffectiveDPFilter::filterNormals(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(vertices);  // TODO
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    int first = triangles(i, 0);
    int second = triangles(i, 1);
    int third = triangles(i, 2);

    for (int j = 0; j < neighbours.at(first).length(); j++) {
      QVector3D neighbourNormal = normals[neighbours[first][j]];
      if (QVector3D().dotProduct(neighbourNormal, normals[i]) > threshold) {
        filteredNormals[i] +=
            (QVector3D().dotProduct(neighbourNormal, normals[i]) - threshold) *
            neighbourNormal;
      }
    }
    for (int j = 0; j < neighbours.at(second).length(); j++) {
      int thisNeighbour = neighbours.at(second).at(j);
      QVector3D neighbourNormal = normals[thisNeighbour];
      bool isNewTriangle = !(neighbours.at(first).contains(thisNeighbour));
      if (QVector3D().dotProduct(neighbourNormal, normals[i]) > threshold &&
          isNewTriangle) {
        filteredNormals[i] +=
            (QVector3D().dotProduct(neighbourNormal, normals[i]) - threshold) *
            neighbourNormal;
      }
    }
    for (int j = 0; j < neighbours.at(third).length(); j++) {
      int thisNeighbour = neighbours.at(third).at(j);
      QVector3D neighbourNormal = normals[thisNeighbour];
      bool isNewTriangle = !(neighbours.at(first).contains(thisNeighbour)) &&
                           !(neighbours.at(second).contains(thisNeighbour));
      if (QVector3D().dotProduct(neighbourNormal, normals[i]) > threshold &&
          isNewTriangle) {
        filteredNormals[i] +=
            (QVector3D().dotProduct(neighbourNormal, normals[i]) - threshold) *
            neighbourNormal;
      }
    }
  }
}

void FastEffectiveDPFilter::updateVertices(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));

    filteredNormals[i].normalize();
    QVector3D projection =
        QVector3D().dotProduct((centroids[i] - first), filteredNormals[i]) *
        filteredNormals[i];
    differences[triangles(i, 0)] += projection;
    projection =
        QVector3D().dotProduct((centroids[i] - second), filteredNormals[i]) *
        filteredNormals[i];
    differences[triangles(i, 1)] += projection;
    projection =
        QVector3D().dotProduct((centroids[i] - third), filteredNormals[i]) *
        filteredNormals[i];
    differences[triangles(i, 2)] += projection;
  }
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    differences[i] /= neighbours[i].length();
    vertices(i, 0) += differences[i].x();
    vertices(i, 1) += differences[i].y();
    vertices(i, 2) += differences[i].z();
  }
}

void FastEffectiveDPFilter::setUpNeighbours(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    neighbours.append(QList<int>());
    differences.append(QVector3D());
  }
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    neighbours[triangles(i, 0)].append(i);
    neighbours[triangles(i, 1)].append(i);
    neighbours[triangles(i, 2)].append(i);
  }
}
