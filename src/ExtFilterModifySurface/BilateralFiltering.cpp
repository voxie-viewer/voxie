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

#include "BilateralFiltering.hpp"

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

BilateralFiltering::BilateralFiltering(double sigmaS, double sigmaP) {
  QList<QSet<int>> neighbourNodes;
  QList<QSet<int>> neighbourTriangles;
  sigmaSpacial = sigmaS;
  sigmaPrediction = sigmaP;
  meanEdgeLength = 0;
}

void BilateralFiltering::compute(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    neighbourNodes.append(QSet<int>());
    neighbourTriangles.append(QSet<int>());
  }

  setUpData(vertices, triangles);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.08, vx::emptyOptions()));
  sigmaSpacial = sigmaSpacial * meanEdgeLength;
  sigmaPrediction = sigmaPrediction * meanEdgeLength;
  mollifyNormals(vertices, triangles, prog);
  evaluateNewPositions(vertices, triangles, sigmaSpacial, sigmaPrediction,
                       prog);
}

void BilateralFiltering::mollifyNormals(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  QList<QVector3D> differences;
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    QVector3D node(vertices(i, 0), vertices(i, 1), vertices(i, 2));
    QSet<int> neighbourhood;
    QVector3D diff;
    double weight = 0;

    getNeighbourhood(i, neighbourhood, 4);
    QSetIterator<int> it(neighbourhood);
    while (it.hasNext()) {
      int next = it.next();
      QVector3D diffThis;
      double weightThis;
      evaluatePositionDiff(node, centroids[next], trianglesAreas[next],
                           centroids[next], sigmaSpacial, sigmaSpacial / 2,
                           diffThis, weightThis);
      diff += diffThis;
      weight += weightThis;
    }
    differences.append(diff / weight);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        0.08 + (0.46 * (double)i / (double)vertices.size<0>()),
        vx::emptyOptions()));
  }
  calculateNormals(vertices, triangles, differences);
}

void BilateralFiltering::evaluateNewPositions(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles,
    double sigmaSp, double SigmaPred,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  Q_UNUSED(triangles);
  Q_UNUSED(sigmaSp);
  Q_UNUSED(SigmaPred);
  QList<QVector3D> differences;
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    QVector3D node(vertices(i, 0), vertices(i, 1), vertices(i, 2));
    QSet<int> neighbourhood;
    QVector3D diff;
    double weight = 0;

    getNeighbourhood(i, neighbourhood, 5);
    QSetIterator<int> it(neighbourhood);
    while (it.hasNext()) {
      int next = it.next();
      QVector3D diffThis;
      double weightThis;
      QVector3D prediction;
      // Plane is given by centroid and mollified normal
      double distance =
          node.distanceToPlane(centroids[next], mollifiedNormals[next]);
      prediction = node - distance * mollifiedNormals[next];
      evaluatePositionDiff(node, prediction, trianglesAreas[next],
                           centroids[next], sigmaSpacial, sigmaPrediction,
                           diffThis, weightThis);
      diff += diffThis;
      weight += weightThis;
    }
    differences.append(diff / weight);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        0.54 + (0.46 * i / (double)vertices.size<0>()), vx::emptyOptions()));
  }
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    vertices(i, 0) = differences[i].x();
    vertices(i, 1) = differences[i].y();
    vertices(i, 2) = differences[i].z();
  }
}

void BilateralFiltering::evaluatePositionDiff(
    QVector3D oldPos, QVector3D prediction, double area, QVector3D centroid,
    double sigmaSp, double SigmaPred, QVector3D& newPos, double& weight) {
  weight = area * evaluateGaussian(oldPos, centroid, sigmaSp) *
           evaluateGaussian(oldPos, prediction, SigmaPred);
  newPos = prediction * weight;
}

void BilateralFiltering::calculateNormals(vx::Array2<float> vertices,
                                          vx::Array2<const uint32_t> triangles,
                                          QList<QVector3D> differences) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    QVector3D first(
        vertices(triangles(i, 0), 0) + differences[triangles(i, 0)].x(),
        vertices(triangles(i, 0), 1) + differences[triangles(i, 0)].y(),
        vertices(triangles(i, 0), 2) + differences[triangles(i, 0)].z());
    QVector3D second(
        vertices(triangles(i, 1), 0) + differences[triangles(i, 1)].x(),
        vertices(triangles(i, 1), 1) + differences[triangles(i, 1)].y(),
        vertices(triangles(i, 1), 2) + differences[triangles(i, 1)].z());
    QVector3D third(
        vertices(triangles(i, 2), 0) + differences[triangles(i, 2)].x(),
        vertices(triangles(i, 2), 1) + differences[triangles(i, 2)].y(),
        vertices(triangles(i, 2), 2) + differences[triangles(i, 2)].z());

    QVector3D normal =
        QVector3D().normal(first * 500, second * 500, third * 500);
    mollifiedNormals.append(normal);
  }
}

double BilateralFiltering::evaluateGaussian(QVector3D point, QVector3D diff,
                                            double sigma) {
  double result = 0.0;
  result = exp(-(diff - point).lengthSquared() / (2 * sigma * sigma));
  return result;
}

void BilateralFiltering::getNeighbourhood(int node, QSet<int>& neighbourhood,
                                          int circleSize) {
  if (circleSize == 0) return;
  QSet<int> toDoNodes;
  toDoNodes.unite(neighbourNodes[node]);
  neighbourhood.unite(neighbourTriangles[node]);
  for (int i = 1; i < circleSize; i++) {
    QSetIterator<int> it(toDoNodes);
    QSet<int> newNodes;
    while (it.hasNext()) {
      int next = it.next();
      neighbourhood.unite(neighbourTriangles[next]);
      newNodes.unite(neighbourNodes[next]);
    }
    toDoNodes.unite(newNodes);
  }
}

void BilateralFiltering::getTriangleInfo(vx::Array2<float> vertices,
                                         vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));

    QVector3D direction = first - second;
    direction = direction * 5000;
    direction.normalize();
    float area = (first - second).length() *
                 third.distanceToLine(first, direction) / 2.0;
    trianglesAreas.append(area);
    meanEdgeLength += first.distanceToPoint(second);
    meanEdgeLength += first.distanceToPoint(third);
    meanEdgeLength += second.distanceToPoint(third);

    centroids.append(QVector3D((first.x() + second.x() + third.x()) / 3,
                               (first.y() + second.y() + third.y()) / 3,
                               (first.z() + second.z() + third.z()) / 3));
  }
  meanEdgeLength /= triangles.size<0>() * 3;
}

void BilateralFiltering::setUpData(vx::Array2<float> vertices,
                                   vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    neighbourNodes[triangles(i, 0)].insert(triangles(i, 1));
    neighbourNodes[triangles(i, 0)].insert(triangles(i, 2));
    neighbourNodes[triangles(i, 1)].insert(triangles(i, 0));
    neighbourNodes[triangles(i, 1)].insert(triangles(i, 2));
    neighbourNodes[triangles(i, 2)].insert(triangles(i, 0));
    neighbourNodes[triangles(i, 2)].insert(triangles(i, 1));

    neighbourTriangles[triangles(i, 0)].insert(i);
    neighbourTriangles[triangles(i, 1)].insert(i);
    neighbourTriangles[triangles(i, 2)].insert(i);
  }
  getTriangleInfo(vertices, triangles);
}
