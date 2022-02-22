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

#ifndef FEATURECONVINCEDDENOISING_H
#define FEATURECONVINCEDDENOISING_H

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>

#include "Edge.hpp"

struct Triangle {
  int first;
  int second;
  int third;
};
struct Node {
  int vertex;
  QSet<Edge> edges;
  QList<int> triangles;
  Node() {
    QSet<Edge> edges;
    QList<int> triangles;
  }
};

class FeatureConvincedDenoising {
 public:
  FeatureConvincedDenoising(vx::Array2<float> vertices,
                            vx::Array2<const uint32_t> triangles);
  void compute(vx::Array2<float>& vertices,
               vx::Array2<const uint32_t>& triangles, int maxExtension,
               vx::ClaimedOperation<
                   de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);

 private:
  QList<QVector3D> verticesCopy;
  QList<Triangle> trianglesCopy;

  QList<Edge> edges;
  QList<int> candidateEdges;
  QList<QMap<int, Edge>> neighbours;
  QList<Node> nodes;
  QList<QVector3D> normals;
  QList<bool> visited;
  double meanLaplacian;
  double meanLength;

  QList<QVector3D> laplacians;
  QList<int> lapNeighbours;

  QList<QList<int>> isotropicTriangleNeighbourhoods;
  QList<QVector3D> normalsForFilter;
  QList<QVector3D> filteredNormals;
  QList<QVector3D> guidingNormals;
  QList<QVector3D> centroids;
  QList<double> areas;
  QList<QVector3D> diffs;

  //***algorithm parameters***
  // for building the feature lines
  double minFeatureEdgeAngle;
  double lineAngleThreshold;
  double extensionAngle;
  int maxExtensionSteps;
  int minimumLineLenghth;

  // parameters for the other filters
  int fedIterations;
  double fedLambda;
  int taubinIterations;

  // for filtering
  int filterIterations;
  int bilateralFilterIterations;
  double sigmaSpacial;
  double sigmaSignal;
  int maxNeighbourhoodSize;

  // set up data
  void createCopies(vx::Array2<float> vertices,
                    vx::Array2<const uint32_t> triangles);
  void setUpNeighbours(vx::Array2<float> vertices,
                       vx::Array2<const uint32_t> triangles);
  void setUpEdge(int one, int two, int triangle);
  void calculateNormals(vx::Array2<float> vertices,
                        vx::Array2<const uint32_t> triangles);

  void applyFilterResult(vx::Array2<float>& vertices);

  // filtering
  void filter(vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
              vx::ClaimedOperation<
                  de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  QList<int> buildIsotropicVertexNeighbourhood(int vertex);
  QList<int> buildIsotropicTriangleNeighbourhood(int triangle);
  QList<int> buildFeatureAwareNeighbourhood(int triangle);
  void generateGuidingNormals();
  void filterNormals();
  void calculateTriangleInfo();
  void calculateDiffs();
  void updateVertices();
  double evaluateGaussian(double difference, double sigma);

  // form feature lines
  void findCandidateEdges(double threshold);
  void formFeatureLines(vx::Array2<float> vertices);
  void insertEdge(vx::Array2<float> vertices, int start);
  void appendAtVertex(vx::Array2<float> vertices, int edge, int vertex);
  void refineLines(vx::Array2<float> vertices);
  void resetFeatureEdges(vx::Array2<float> vertices);
  void resetLine(vx::Array2<float> vertices, int head);
  void extendLines(vx::Array2<float> vertices);
  void extendLine(vx::Array2<float> vertices, int head);
  bool extendLineEnd(vx::Array2<float> vertices, int edge, int vertex,
                     int remainingSteps);
  void removeWrongFeatureEdges();

  // feature line navigation
  int followToLineHead(int index);
  int followPathToBeCutOff(vx::Array2<float> vertices, double& dToBeCutOff,
                           double& iToBeCutOff, int& lengthToBeCutOff, int edge,
                           int awayFromNode);
  void setNewHead(vx::Array2<float> vertices, int newHead, double& dToBeCutOff,
                  double& iToBeCutOff, int& lengthToBeCutOff, int edge,
                  int awayFromNode);
  int traverseLineToOppositeEnd(vx::Array2<float> vertices, int start,
                                int awayFromNode);

  // small helper methods
  double angleBetweenEdges(vx::Array2<float> vertices, Edge e1, Edge e2);
  int findSharedVertex(int eOne, int eTwo);
  QVector3D calculateNeighbourhoodNormal(int vertex, QList<int> triangles);
  void setParameters(
      vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
      vx::ClaimedOperation<
          de::uni_stuttgart::Voxie::ExternalOperationRunFilter>& prog);
  void computeNoise(vx::Array2<float> vertices,
                    vx::Array2<const uint32_t> triangles, double& msel);
  void resetVertices(vx::Array2<float>& vertices);
};

#endif  // FEATURECONVINCEDDENOISING_H
