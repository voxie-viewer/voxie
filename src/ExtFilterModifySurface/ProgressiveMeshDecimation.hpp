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

#ifndef PROGRESSIVEMESHDECIMATION_H
#define PROGRESSIVEMESHDECIMATION_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

class ProgressiveMeshDecimation {
 public:
  ProgressiveMeshDecimation();
  void compute(vx::Array2<const float> vertices,
               vx::Array2<const uint32_t> triangles, double percentage,
               double angle,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void copyResults(vx::Array2<const float> vertices,
                   vx::Array2<uint32_t>& triangles);
  void getTrianglesResults(vx::Array2<uint32_t>& triangles);
  void getVerticesResults(vx::Array2<float>& vertices);
  int getRemainingVertexCount();
  int getRemainingTriangleCount();

 private:
  // parameters
  double featureThreshold;
  int verticesEliminated;
  int trianglesEliminated;

  // Vertex Eigenschaften
  QVector<QVector3D> positions;
  QList<QList<int>> neighbourTriangles;
  QList<bool> isNonManifold;
  QList<bool> isBoundary;
  QList<bool> isSimple;
  QList<bool> stillExistsVertex;
  QVector<double> errors;

  QList<bool> stillExistsTriangle;
  QList<QList<int>> trianglesCopy;
  QList<QList<float>> verticesCopy;
  QMap<int, int> verticesIndexMapping;

  QVector<int> priorityQueue;  // holds indices of Vertices
  QList<int> indicesInQueue;   // holds current indices in Queue

  int insertIntoQueue(int index, int pos);
  void deleteFromQueue(int index);
  void removeTopFromQueue();

  void setUpData(vx::Array2<const float> vertices,
                 vx::Array2<const uint32_t> triangles);
  void copyTriangles(vx::Array2<const uint32_t> triangles);
  void copyVertices(vx::Array2<const float> vertices);
  void mapVertices(vx::Array2<const float> vertices);

  void classifyVertices(vx::Array2<const uint32_t> triangles);
  void classify(int index, vx::Array2<const uint32_t> triangles);

  void calculateErrors(vx::Array2<const uint32_t> triangles);
  double calculateError(int index, vx::Array2<const uint32_t> triangles);

  double calculateTriangleArea(int i, vx::Array2<const uint32_t> triangles);
  QVector3D calculateCentroid(int i, vx::Array2<const uint32_t> triangles);
  QVector3D calculateNormal(int i, vx::Array2<const uint32_t> triangles);

  void decimate(
      int threshold, vx::Array2<const uint32_t> triangles,
      vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void eliminate(int index, vx::Array2<const uint32_t> triangles);
  void collapseSimple(int index, QList<int> neighbourSet,
                      vx::Array2<const uint32_t> triangles);
  void collapseBoundary(int index, QList<int> candidates, QList<int> neighbours,
                        vx::Array2<const uint32_t> triangles);
  bool isValidTriangulation(int index, int toBeCollapsed,
                            QList<int> neighbourSet,
                            vx::Array2<const uint32_t> triangles);
  void buildSubLoop(int ignoreOne, int ignoreTwo, int current,
                    QList<int> neighbours, QList<QList<int>> loopNeighbours,
                    QList<int>& subLoop);
  bool allOnSameSide(QList<int> vertices, QVector3D dirOne, QVector3D dirTwo,
                     int vertexOnPlane, bool& abovePlane);
  void edgeCollapse(int fromVertex, int toVertex, QList<int> neighbours,
                    vx::Array2<const uint32_t> triangles);
};

#endif  // PROGRESSIVEMESHDECIMATION_H
