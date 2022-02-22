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

#include "FeatureConvincedDenoising.hpp"

#include <cmath>

#include "FastEffectiveDPFilter.hpp"
#include "TaubinFiltering.hpp"

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

FeatureConvincedDenoising::FeatureConvincedDenoising(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(vertices);   // TODO
  Q_UNUSED(triangles);  // TODO
}

void FeatureConvincedDenoising::compute(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t>& triangles,
    int maxExtensions,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  maxExtensionSteps = maxExtensions;

  createCopies(vertices, triangles);

  setParameters(vertices, triangles, prog);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.18, vx::emptyOptions()));

  TaubinFiltering taubin(taubinIterations, 0.6);
  taubin.compute(vertices, triangles, prog);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.22, vx::emptyOptions()));
  FastEffectiveDPFilter fed;
  fed.compute(vertices, triangles, 0.4, fedIterations, prog);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.32, vx::emptyOptions()));
  qDebug() << "FED completed";
  setUpNeighbours(vertices, triangles);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.35, vx::emptyOptions()));
  qDebug() << "neighbours set up";
  calculateNormals(vertices, triangles);

  findCandidateEdges(minFeatureEdgeAngle);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.35, vx::emptyOptions()));
  formFeatureLines(vertices);

  refineLines(vertices);
  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(0.38, vx::emptyOptions()));

  filter(vertices, triangles, prog);
  applyFilterResult(vertices);
}

void FeatureConvincedDenoising::applyFilterResult(vx::Array2<float>& vertices) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    vertices(i, 0) = verticesCopy.at(i).x();
    vertices(i, 1) = verticesCopy.at(i).y();
    vertices(i, 2) = verticesCopy.at(i).z();
  }
}

void FeatureConvincedDenoising::filter(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  Q_UNUSED(vertices);   // TODO
  Q_UNUSED(triangles);  // TODO
  for (int i = 0; i < trianglesCopy.size(); i++) {
    isotropicTriangleNeighbourhoods.append(
        buildIsotropicTriangleNeighbourhood(i));
  }
  for (int i = 0; i < filterIterations; i++) {
    calculateTriangleInfo();
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        0.38 + 0.62 * (double)(i + 0.05) / (double)filterIterations,
        vx::emptyOptions()));
    generateGuidingNormals();
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        0.38 + 0.62 * (double)(i + 0.25) / (double)filterIterations,
        vx::emptyOptions()));
    filterNormals();
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        0.38 + 0.62 * (double)(i + 0.8) / (double)filterIterations,
        vx::emptyOptions()));
    calculateDiffs();
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        0.38 + 0.62 * (double)(i + 0.95) / (double)filterIterations,
        vx::emptyOptions()));
    updateVertices();
  }
}

void FeatureConvincedDenoising::updateVertices() {
  for (int i = 0; i < verticesCopy.size(); i++) {
    if (diffs.at(i).length() < meanLength) {
      verticesCopy[i] = verticesCopy.at(i) + diffs.at(i);
    }
  }
}

void FeatureConvincedDenoising::filterNormals() {
  for (int j = 0; j < bilateralFilterIterations; j++) {
    for (int i = 0; i < trianglesCopy.size(); i++) {
      QVector3D filteredNormal(0, 0, 0);

      QList<int> neighbourhood = isotropicTriangleNeighbourhoods.at(i);

      for (int k = 0; k < neighbourhood.size(); k++) {
        double weight = 0;

        double distance =
            (centroids.at(i) - centroids.at(neighbourhood.at(k))).length();
        double normalsDifference =
            (guidingNormals.at(i) - guidingNormals.at(neighbourhood.at(k)))
                .length();

        weight = areas.at(neighbourhood.at(k));
        weight = weight * evaluateGaussian(distance, sigmaSpacial);
        weight = weight * evaluateGaussian(normalsDifference, sigmaSignal);

        filteredNormal += weight * normalsForFilter.at(neighbourhood.at(k));
      }
      filteredNormal = filteredNormal * 20000;
      filteredNormal.normalize();
      filteredNormals.append(filteredNormal);
    }

    for (int i = 0; i < trianglesCopy.size(); i++) {
      normalsForFilter[i] = filteredNormals.at(i);
    }
    filteredNormals.clear();
  }
  for (int i = 0; i < trianglesCopy.size(); i++) {
    filteredNormals.append(normalsForFilter.at(i));
  }
}

void FeatureConvincedDenoising::calculateDiffs() {
  for (int i = 0; i < verticesCopy.size(); i++) {
    QList<int> neighbourhood = buildIsotropicVertexNeighbourhood(i);
    if (neighbourhood.size() < 4) {
      diffs.append(QVector3D(0, 0, 0));
      continue;
    }
    QVector3D diff(0, 0, 0);

    for (int j = 0; j < neighbourhood.size(); j++) {
      double weight = QVector3D().dotProduct(
          filteredNormals.at(neighbourhood.at(j)),
          (centroids.at(neighbourhood.at(j)) - verticesCopy.at(i)));
      diff = diff + weight * filteredNormals.at(neighbourhood.at(j));
    }
    diff = diff / neighbourhood.size();
    diffs.append(diff);
  }
}

double FeatureConvincedDenoising::evaluateGaussian(double difference,
                                                   double sigma) {
  double result = 0.0;
  result = exp(-(difference * difference) / (2 * sigma * sigma));
  return result;
}

void FeatureConvincedDenoising::generateGuidingNormals() {
  for (int i = 0; i < trianglesCopy.size(); i++) {
    QList<int> neighbourhood = buildFeatureAwareNeighbourhood(i);

    QVector3D normal(0, 0, 0);

    for (int j = 0; j < neighbourhood.size(); j++) {
      int index = neighbourhood.at(j);
      normal = normal + (areas.at(index) * normalsForFilter.at(index));
    }
    normal = normal * 2000;
    normal.normalize();
    guidingNormals.append(normal);
  }
}

void FeatureConvincedDenoising::calculateTriangleInfo() {
  diffs.clear();
  centroids.clear();
  normalsForFilter.clear();
  guidingNormals.clear();
  filteredNormals.clear();
  areas.clear();

  for (int i = 0; i < trianglesCopy.size(); i++) {
    QVector3D first = verticesCopy.at(trianglesCopy.at(i).first);
    QVector3D second = verticesCopy.at(trianglesCopy.at(i).second);
    QVector3D third = verticesCopy.at(trianglesCopy.at(i).third);

    QVector3D centroid = (first + second + third) / 3;
    centroids.append(centroid);

    QVector3D normal =
        QVector3D().normal(first * 500, second * 500, third * 500);
    normalsForFilter.append(normal);

    QVector3D direction = first - second;
    direction = direction * 5000;
    direction.normalize();
    double area = (first - second).length() *
                  third.distanceToLine(first, direction) / 2.0;
    areas.append(area);
  }
}

/**
 * @brief FeatureConvincedDenoising::buildIsotropicNeighbourhood bildet eine
 * isotrope Nachbarschaft um vertex, diese beinhaltet den 1-Ring um Vertex sowie
 * die Dreiecke jenseits der nicht anliegenden Kante dieser Dreiecke
 * @param vertex
 * @return
 */
QList<int> FeatureConvincedDenoising::buildIsotropicVertexNeighbourhood(
    int vertex) {
  QList<int> neighbourTriangles;

  QList<int> neighbourVertices;
  QList<Edge> _edges = nodes.at(vertex).edges.values();

  for (int i = 0; i < _edges.size(); i++) {
    int edge = _edges.at(i).index;
    if (edges.at(edge).vertexOne == vertex) {
      neighbourVertices.append(edges.at(edge).vertexTwo);
    } else if (edges.at(edge).vertexTwo == vertex) {
      neighbourVertices.append(edges.at(edge).vertexOne);
    } else {
      qDebug() << "not a neighbouring edge!";
    }
  }
  for (int i = 0; i < neighbourVertices.size(); i++) {
    QList<Edge> theseEdges = nodes.at(neighbourVertices.at(i)).edges.values();
    for (int j = 0; j < theseEdges.size(); j++) {
      int edge = theseEdges.at(j).index;
      if ((neighbourVertices.contains(edges.at(edge).vertexOne)) &&
          (neighbourVertices.contains(edges.at(edge).vertexTwo)) &&
          !(neighbourTriangles.contains(edges.at(edge).triangleOne))) {
        neighbourTriangles.append(edges.at(edge).triangleOne);
        neighbourTriangles.append(edges.at(edge).triangleTwo);
      }
    }
  }

  return neighbourTriangles;
}

QList<int> FeatureConvincedDenoising::buildIsotropicTriangleNeighbourhood(
    int triangle) {
  QList<int> neighbourhood;
  QList<int> facetQueue;

  neighbourhood.append(triangle);
  facetQueue.append(triangle);

  while (!(facetQueue.isEmpty()) &&
         (neighbourhood.size() < maxNeighbourhoodSize)) {
    int facet = facetQueue.first();
    facetQueue.removeAt(0);

    QList<int> vertices;
    vertices.append(trianglesCopy.at(facet).first);
    vertices.append(trianglesCopy.at(facet).second);
    vertices.append(trianglesCopy.at(facet).third);

    // for each vertex, find edge in triangle, append other triangle of that
    // edge if non-feature edge add if one of the triangles is already in the nh
    // and the other isn't
    for (int i = 0; i < 3; i++) {
      QList<Edge> edgesThis = nodes.at(vertices.at(i)).edges.values();
      for (int j = 0; j < edgesThis.size(); j++) {
        int edge = edgesThis.at(j).index;

        if (neighbourhood.contains(edges.at(edge).triangleOne) &&
            !(neighbourhood.contains(edges.at(edge).triangleTwo))) {
          neighbourhood.append(edges.at(edge).triangleTwo);
          facetQueue.append(edges.at(edge).triangleTwo);
          if (neighbourhood.size() == maxNeighbourhoodSize) {
            return neighbourhood;
          }
        } else if (neighbourhood.contains(edges.at(edge).triangleTwo) &&
                   !(neighbourhood.contains(edges.at(edge).triangleOne))) {
          neighbourhood.append(edges.at(edge).triangleOne);
          facetQueue.append(edges.at(edge).triangleOne);
          if (neighbourhood.size() == maxNeighbourhoodSize) {
            return neighbourhood;
          }
        }
      }
    }
  }
  return neighbourhood;
}

/**
 * @brief FeatureConvincedDenoising::buildFeatureAwareNeighbourhood erstellt
 * eine Nachbarschaft um das Dreieck triangle ohne dabei eine Feature-Edge zu
 * überqueren, diese ist höchstens maxNeighbourhoodSize groß, kleiner falls das
 * Dreieck enger von Feature-Edges umgeben ist
 * @param triangle index des Dreiecks
 * @return die Liste mit den Indizes der Dreiecke in der Nachbarschaft
 */
QList<int> FeatureConvincedDenoising::buildFeatureAwareNeighbourhood(
    int triangle) {
  QList<int> neighbourhood;
  QList<int> facetQueue;

  neighbourhood.append(triangle);
  facetQueue.append(triangle);

  while (!(facetQueue.isEmpty()) &&
         (neighbourhood.size() < maxNeighbourhoodSize)) {
    int facet = facetQueue.first();
    facetQueue.removeAt(0);

    QList<int> vertices;
    vertices.append(trianglesCopy.at(facet).first);
    vertices.append(trianglesCopy.at(facet).second);
    vertices.append(trianglesCopy.at(facet).third);

    // for each vertex, find edge in triangle, append other triangle of that
    // edge if non-feature edge add if one of the triangles is already in the nh
    // and the other isn't
    for (int i = 0; i < 3; i++) {
      QList<Edge> edgesThis = nodes.at(vertices.at(i)).edges.values();
      for (int j = 0; j < edgesThis.size(); j++) {
        int edge = edgesThis.at(j).index;

        if (edges.at(edge).feature || edges.at(edge).extension) {
          continue;
        }

        if (neighbourhood.contains(edges.at(edge).triangleOne) &&
            !(neighbourhood.contains(edges.at(edge).triangleTwo))) {
          neighbourhood.append(edges.at(edge).triangleTwo);
          facetQueue.append(edges.at(edge).triangleTwo);
          if (neighbourhood.size() == maxNeighbourhoodSize) {
            return neighbourhood;
          }
        } else if (neighbourhood.contains(edges.at(edge).triangleTwo) &&
                   !(neighbourhood.contains(edges.at(edge).triangleOne))) {
          neighbourhood.append(edges.at(edge).triangleOne);
          facetQueue.append(edges.at(edge).triangleOne);
          if (neighbourhood.size() == maxNeighbourhoodSize) {
            return neighbourhood;
          }
        }
      }
    }
  }
  return neighbourhood;
}

void FeatureConvincedDenoising::findCandidateEdges(double threshold) {
  double threshAngle = std::acos(threshold);
  for (int i = 0; i < edges.size(); i++) {
    Edge e = edges[i];
    edges[i].angleCosine = std::abs(QVector3D().dotProduct(
        normals.at(e.triangleOne), normals.at(e.triangleTwo)));
    double angle = std::acos(edges[i].angleCosine);
    if (angle > threshAngle) {
      edges[i].feature = true;
      edges[i].lineParent = -1;
      edges[i].saliency = 0;
      edges[i].iLine = 0;
      edges[i].dThisEdge = 0;
      edges[i].lineNeighbourOne = -1;
      edges[i].lineNeighbourTwo = -1;
      edges[i].directionDiffOne = -1;
      edges[i].directionDiffTwo = -1;
      edges[i].lineLength = 0;
      candidateEdges.append(i);
    }
  }
}

void FeatureConvincedDenoising::formFeatureLines(vx::Array2<float> vertices) {
  for (int i = 0; i < candidateEdges.size(); i++) {
    insertEdge(vertices, candidateEdges.at(i));
  }
}

void FeatureConvincedDenoising::insertEdge(vx::Array2<float> vertices,
                                           int index) {
  // diese Kante als (1-)Line aufsetzen
  edges[index].lineParent = index;
  edges[index].lineLength = 1;
  edges[index].dThisEdge = std::abs((normals.at(edges[index].triangleOne) -
                                     normals.at(edges[index].triangleTwo))
                                        .length());
  edges[index].iLine = 0.0;
  edges[index].dLine = edges.at(index).dThisEdge;
  edges[index].saliency = 0.0;

  appendAtVertex(vertices, index, edges.at(index).vertexOne);
  appendAtVertex(vertices, index, edges.at(index).vertexTwo);
}

void FeatureConvincedDenoising::appendAtVertex(vx::Array2<float> vertices,
                                               int edge, int vertex) {
  QList<Edge> e = nodes.at(vertex).edges.values();
  QList<int> candidates;

  for (int i = 0; i < e.length(); i++) {
    int currentEdge = e.at(i).index;
    if (!edges.at(currentEdge).feature) {
      continue;
    }
    if (!(edges.at(currentEdge).lineParent >= 0)) {
      continue;
    }
    if (angleBetweenEdges(vertices, edges.at(currentEdge), edges.at(edge)) <
        lineAngleThreshold) {
      continue;
    }
    if (currentEdge == edge) {
      continue;
    }
    candidates.append(currentEdge);
  }

  double bestSaliency = 0;
  int bestCandidate = -1;

  int thisHead = followToLineHead(edge);

  for (int i = 0; i < candidates.size(); i++) {
    int currentHead = followToLineHead(candidates.at(i));

    double dPot = 0;
    double iPot = 0;
    int lengthPot = 0;

    followPathToBeCutOff(vertices, dPot, iPot, lengthPot, candidates.at(i),
                         vertex);
    dPot = dPot + edges.at(thisHead).dLine;
    //        qDebug() << "Potentielles D: " << dPot;
    iPot = iPot + edges.at(thisHead).iLine * edges.at(thisHead).lineLength +
           std::cos(3.14 - angleBetweenEdges(vertices, edges.at(edge),
                                             edges.at(candidates.at(i))));
    //        qDebug() << "Winkel: " << angleBetweenEdges(vertices,
    //        edges.at(edge), edges.at(candidates.at(i))); qDebug() <<
    //        "Potentielles I: " << iPot;
    iPot = iPot / (edges.at(thisHead).lineLength + lengthPot - 1);
    //        qDebug() << "Potentielles I: " << iPot;
    double saliencyPot = dPot * iPot;
    //        qDebug() << "Potentielle Saliency: " << saliencyPot;
    //        qDebug() << "Aktuelle Saliency: " <<
    //        edges.at(currentHead).saliency;

    if (saliencyPot > bestSaliency &&
        saliencyPot > edges.at(currentHead).saliency) {
      bestSaliency = saliencyPot;
      bestCandidate = candidates.at(i);
    }
  }

  //(re-)arrange the line
  if (bestCandidate >= 0 && (bestCandidate != thisHead)) {
    int oldNeighbour = -1;
    if (edges.at(bestCandidate).vertexOne == vertex) {
      oldNeighbour = edges.at(bestCandidate).lineNeighbourOne;
    } else {
      oldNeighbour = edges.at(bestCandidate).lineNeighbourTwo;
    }

    if (oldNeighbour > -1) {
      if (edges.at(oldNeighbour).vertexOne == vertex) {
        edges[oldNeighbour].lineNeighbourOne = -1;
      } else {
        edges[oldNeighbour].lineNeighbourTwo = -1;
      }
      double dNei = 0;
      double iNei = 0;
      int lengthNei = 0;
      setNewHead(vertices, oldNeighbour, dNei, iNei, lengthNei, oldNeighbour,
                 vertex);
    }

    double dCan = 0;
    double iCan = 0;
    int lengthCan = 0;
    int newHead = followPathToBeCutOff(vertices, dCan, iCan, lengthCan,
                                       bestCandidate, vertex);

    if (edges.at(edge).vertexOne == vertex) {
      edges[edge].lineNeighbourOne = bestCandidate;
    } else if (edges.at(edge).vertexTwo == vertex) {
      edges[edge].lineNeighbourTwo = bestCandidate;
    } else {
      qDebug() << "not a neighbour!";
    }

    if (edges.at(bestCandidate).vertexOne == vertex) {
      edges[bestCandidate].lineNeighbourOne = edge;
    } else if (edges.at(bestCandidate).vertexTwo == vertex) {
      edges[bestCandidate].lineNeighbourTwo = edge;
    } else {
      qDebug() << "not a neighbour!";
    }

    double dNew = 0;
    double iNew = 0;
    int lengthNew = 0;

    int lastVertex = -1;
    if (edges.at(newHead).lineNeighbourOne == -1) {
      lastVertex = edges.at(newHead).vertexOne;
    } else {
      lastVertex = edges.at(newHead).vertexTwo;
    }
    setNewHead(vertices, newHead, dNew, iNew, lengthNew, newHead, lastVertex);
  }
}

void FeatureConvincedDenoising::refineLines(vx::Array2<float> vertices) {
  //    resetFeatureEdges(vertices);
  extendLines(vertices);
  removeWrongFeatureEdges();
}

// TODO: remove?
void FeatureConvincedDenoising::resetFeatureEdges(vx::Array2<float> vertices) {
  for (int i = 0; i < candidateEdges.size(); i++) {
    int edge = candidateEdges.at(i);
    // entspricht for each (proper) feature line
    if ((edges.at(edge).lineParent == edge) &&
        (edges.at(edge).lineLength > 10)) {
      resetLine(vertices, edge);
    }
  }
}

// TODO: remove?
void FeatureConvincedDenoising::resetLine(vx::Array2<float> vertices,
                                          int head) {
  Q_UNUSED(vertices);  // TODO
  Q_UNUSED(head);      // TODO
}

void FeatureConvincedDenoising::extendLines(vx::Array2<float> vertices) {
  for (int i = 0; i < candidateEdges.size(); i++) {
    int edge = candidateEdges.at(i);
    // entspricht for each (proper) feature line
    if ((edges.at(edge).lineParent == edge) &&
        (edges.at(edge).lineLength > minimumLineLenghth)) {
      extendLine(vertices, edge);
    }
  }
}

void FeatureConvincedDenoising::extendLine(vx::Array2<float> vertices,
                                           int head) {
  int dartEnd = -1;
  if (edges.at(head).lineNeighbourOne < 0) {
    dartEnd = edges.at(head).vertexOne;
  } else if (edges.at(head).lineNeighbourTwo < 0) {
    dartEnd = edges.at(head).vertexTwo;
  }
  int oppositeEnd = traverseLineToOppositeEnd(vertices, head, dartEnd);
  int opDartEnd = -1;

  if (edges.at(oppositeEnd).lineNeighbourOne < 0) {
    opDartEnd = edges.at(oppositeEnd).vertexOne;
  } else if (edges.at(oppositeEnd).lineNeighbourTwo < 0) {
    opDartEnd = edges.at(oppositeEnd).vertexTwo;
  }

  if (extendLineEnd(vertices, head, dartEnd, maxExtensionSteps)) {
  }
  if (extendLineEnd(vertices, oppositeEnd, opDartEnd, maxExtensionSteps)) {
  }
}

bool FeatureConvincedDenoising::extendLineEnd(vx::Array2<float> vertices,
                                              int edge, int vertex,
                                              int remainingSteps) {
  QList<Edge> neighbourEdges = nodes.at(vertex).edges.values();
  if (remainingSteps == 0) {
    return false;
  }

  for (int i = 0; i < neighbourEdges.size(); i++) {
    int neighbourEdge = neighbourEdges.at(i).index;
    if (neighbourEdge == edge) {
      continue;
    }
    if (angleBetweenEdges(vertices, edges.at(edge), edges.at(neighbourEdge)) >
        extensionAngle) {
      if (edges.at(neighbourEdge).feature &&
          edges.at(followToLineHead(neighbourEdge)).lineLength >
              minimumLineLenghth) {
        edges[edge].extension = true;
        return true;
      }
      int otherVertex = -1;
      if (edges.at(neighbourEdge).vertexOne == vertex) {
        otherVertex = edges.at(neighbourEdge).vertexTwo;
      } else if (edges.at(neighbourEdge).vertexTwo == vertex) {
        otherVertex = edges.at(neighbourEdge).vertexOne;
      }
      if (extendLineEnd(vertices, neighbourEdge, otherVertex,
                        remainingSteps - 1)) {
        edges[neighbourEdge].extension = true;
        return true;
      }
    }
  }
  return false;
}

void FeatureConvincedDenoising::removeWrongFeatureEdges() {
  for (int i = 0; i < candidateEdges.size(); i++) {
    int head = followToLineHead(candidateEdges.at(i));
    if (edges.at(head).lineLength < minimumLineLenghth) {
      edges[candidateEdges.at(i)].feature = false;
    }
  }
}

int FeatureConvincedDenoising::findSharedVertex(int eOne, int eTwo) {
  if ((edges.at(eOne).vertexOne == edges.at(eTwo).vertexOne) ||
      (edges.at(eOne).vertexOne == edges.at(eTwo).vertexTwo)) {
    return edges.at(eOne).vertexOne;
  } else if ((edges.at(eOne).vertexTwo == edges.at(eTwo).vertexOne) ||
             (edges.at(eOne).vertexTwo == edges.at(eTwo).vertexTwo)) {
    return edges.at(eOne).vertexTwo;
  } else {
    qDebug() << "no shared Vertex";
    return -1;
  }
}

void FeatureConvincedDenoising::setUpNeighbours(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    neighbours.append(QMap<int, Edge>());
    Node n;
    n.vertex = i;
    nodes.append(n);
  }
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    int one = triangles(i, 0);
    int two = triangles(i, 1);
    int three = triangles(i, 2);

    nodes[one].triangles.append(i);
    nodes[two].triangles.append(i);
    nodes[three].triangles.append(i);

    setUpEdge(one, two, i);
    setUpEdge(one, three, i);
    setUpEdge(two, three, i);
  }
}

void FeatureConvincedDenoising::setUpEdge(int one, int two, int triangle) {
  int ne = 0;
  int co = 0;
  if (neighbours.at(one).contains(two)) {
    Edge e = neighbours[one][two];
    edges[e.index].triangleTwo = triangle;
    edges[e.index].feature = false;
    edges[e.index].extension = false;
    co++;
  } else {
    Edge e;
    e.feature = false;
    e.extension = false;
    e.vertexOne = std::min(one, two);
    e.vertexTwo = std::max(one, two);
    e.triangleOne = triangle;
    e.index = edges.size();
    nodes[one].edges.insert(e);
    nodes[two].edges.insert(e);
    neighbours[one].insert(two, e);
    neighbours[two].insert(one, e);
    e.lineParent = -1;
    e.saliency = 0;
    e.iLine = 0;
    e.dThisEdge = 0;
    e.lineNeighbourOne = -1;
    e.lineNeighbourTwo = -1;
    e.directionDiffOne = -1;
    e.directionDiffTwo = -1;
    e.dLine = 0;
    e.lineLength = 0;
    edges.append(e);
    ne++;
  }
  if (ne == co) {
    qDebug() << "!Fehler!";
  }
}

void FeatureConvincedDenoising::calculateNormals(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));

    QVector3D normal = QVector3D().normal(
        first * 500, second * 500, third * 500);  // TODO: bessere Lösung hier
    normals.append(normal);
  }
}

void FeatureConvincedDenoising::createCopies(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    QVector3D vec(vertices(i, 0), vertices(i, 1), vertices(i, 2));
    verticesCopy.append(vec);
  }
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    Triangle tri;
    tri.first = triangles(i, 0);
    tri.second = triangles(i, 1);
    tri.third = triangles(i, 2);
    trianglesCopy.append(tri);
  }
}

/*
 * ermittelt den Winkel (in Bogenmaß) zwischen zwei Kanten, wenn die Kanten
 * einen Vertex teilen gibt sonst als Winkel 0 zurück
 */
double FeatureConvincedDenoising::angleBetweenEdges(vx::Array2<float> vertices,
                                                    Edge e1, Edge e2) {
  QVector3D vectorOne;
  QVector3D vectorTwo;

  if (e1.vertexOne == e2.vertexOne) {
    vectorOne =
        QVector3D(vertices(e1.vertexTwo, 0) - vertices(e1.vertexOne, 0),
                  vertices(e1.vertexTwo, 1) - vertices(e1.vertexOne, 1),
                  vertices(e1.vertexTwo, 2) - vertices(e1.vertexOne, 2));
    vectorTwo =
        QVector3D(vertices(e2.vertexTwo, 0) - vertices(e2.vertexOne, 0),
                  vertices(e2.vertexTwo, 1) - vertices(e2.vertexOne, 1),
                  vertices(e2.vertexTwo, 2) - vertices(e2.vertexOne, 2));
  } else if (e1.vertexOne == e2.vertexTwo) {
    vectorOne =
        QVector3D(vertices(e1.vertexTwo, 0) - vertices(e1.vertexOne, 0),
                  vertices(e1.vertexTwo, 1) - vertices(e1.vertexOne, 1),
                  vertices(e1.vertexTwo, 2) - vertices(e1.vertexOne, 2));
    vectorTwo =
        QVector3D(vertices(e2.vertexOne, 0) - vertices(e2.vertexTwo, 0),
                  vertices(e2.vertexOne, 1) - vertices(e2.vertexTwo, 1),
                  vertices(e2.vertexOne, 2) - vertices(e2.vertexTwo, 2));
  } else if (e1.vertexTwo == e2.vertexOne) {
    vectorOne =
        QVector3D(vertices(e1.vertexOne, 0) - vertices(e1.vertexTwo, 0),
                  vertices(e1.vertexOne, 1) - vertices(e1.vertexTwo, 1),
                  vertices(e1.vertexOne, 2) - vertices(e1.vertexTwo, 2));
    vectorTwo =
        QVector3D(vertices(e2.vertexTwo, 0) - vertices(e2.vertexOne, 0),
                  vertices(e2.vertexTwo, 1) - vertices(e2.vertexOne, 1),
                  vertices(e2.vertexTwo, 2) - vertices(e2.vertexOne, 2));
  } else if (e1.vertexTwo == e2.vertexTwo) {
    vectorOne =
        QVector3D(vertices(e1.vertexOne, 0) - vertices(e1.vertexTwo, 0),
                  vertices(e1.vertexOne, 1) - vertices(e1.vertexTwo, 1),
                  vertices(e1.vertexOne, 2) - vertices(e1.vertexTwo, 2));
    vectorTwo =
        QVector3D(vertices(e2.vertexOne, 0) - vertices(e2.vertexTwo, 0),
                  vertices(e2.vertexOne, 1) - vertices(e2.vertexTwo, 1),
                  vertices(e2.vertexOne, 2) - vertices(e2.vertexTwo, 2));
  } else {
    return 0.0;
  }
  vectorOne.normalize();
  vectorTwo.normalize();
  double angle = std::acos(QVector3D().dotProduct(vectorOne, vectorTwo));
  return angle;
}

/*
 * findet den Kopf der Feature Line, in der index ist (dieser hat sich selbst
 * als parent setzt auch direkt fuer alle Edges auf dem Weg zum Kopf diesen
 * direkt als parent um die Suchzeit weiter kurz zu halten
 *
 */
int FeatureConvincedDenoising::followToLineHead(int index) {
  if (index < 0) {
    qDebug() << "not a line member";
    return -1;
  }
  if (edges[index].index == edges[index].lineParent) {
    return index;
  }
  edges[index].lineParent = followToLineHead(edges[index].lineParent);
  return edges[index].lineParent;
}

/*
 * folgt der Feature Line weg vom angegebenen Knoten bis zu deren Ende und
 * ermittelt dabei die Eigenschaften dieses Lineabschnitts
 */
int FeatureConvincedDenoising::followPathToBeCutOff(
    vx::Array2<float> vertices, double& dToBeCutOff, double& iToBeCutOff,
    int& lengthToBeCutOff, int edge, int awayFromNode) {
  dToBeCutOff += edges.at(edge).dThisEdge;
  lengthToBeCutOff++;
  if (awayFromNode == edges.at(edge).vertexOne) {
    if (edges.at(edge).lineNeighbourTwo >= 0) {
      iToBeCutOff += std::cos(
          3.14 - angleBetweenEdges(vertices, edges.at(edge),
                                   edges.at(edges.at(edge).lineNeighbourTwo)));
      return followPathToBeCutOff(
          vertices, dToBeCutOff, iToBeCutOff, lengthToBeCutOff,
          edges.at(edge).lineNeighbourTwo, edges.at(edge).vertexTwo);
    }
  }
  if (awayFromNode == edges.at(edge).vertexTwo) {
    if (edges.at(edge).lineNeighbourOne >= 0) {
      iToBeCutOff += std::cos(
          3.14 - angleBetweenEdges(vertices, edges.at(edge),
                                   edges.at(edges.at(edge).lineNeighbourOne)));
      return followPathToBeCutOff(
          vertices, dToBeCutOff, iToBeCutOff, lengthToBeCutOff,
          edges.at(edge).lineNeighbourOne, edges.at(edge).vertexOne);
    }
  }
  return edge;
}

/*
 *  Setzt newHead als neuen Kopf der Linie, die dort an der Kante edge startet
 * und vom Knoten awayFromNode wegfuehrt, updatet sämtliche Groeßen dieser neuen
 * Line und setzt fuer alle Kanten den Parent direkt auf den neuen Kopf
 */
void FeatureConvincedDenoising::setNewHead(vx::Array2<float> vertices,
                                           int newHead, double& newD,
                                           double& newI, int& newLength,
                                           int edge, int awayFromNode) {
  newD += edges.at(edge).dThisEdge;
  edges[edge].lineParent = newHead;
  newLength++;
  if (awayFromNode == edges.at(edge).vertexOne) {
    if (edges.at(edge).lineNeighbourTwo >= 0 &&
        !(edges.at(edge).lineNeighbourTwo == newHead)) {
      newI += std::cos(
          3.14 - angleBetweenEdges(vertices, edges.at(edge),
                                   edges.at(edges.at(edge).lineNeighbourTwo)));
      setNewHead(vertices, newHead, newD, newI, newLength,
                 edges.at(edge).lineNeighbourTwo, edges.at(edge).vertexTwo);
    }
  }
  if (awayFromNode == edges.at(edge).vertexTwo) {
    if (edges.at(edge).lineNeighbourOne >= 0 &&
        !(edges.at(edge).lineNeighbourOne == newHead)) {
      newI += std::cos(
          3.14 - angleBetweenEdges(vertices, edges.at(edge),
                                   edges.at(edges.at(edge).lineNeighbourOne)));
      setNewHead(vertices, newHead, newD, newI, newLength,
                 edges.at(edge).lineNeighbourOne, edges.at(edge).vertexOne);
    }
  }
  edges[newHead].dLine = newD;
  if (newLength > 1) {
    edges[newHead].iLine = newI / (newLength - 1);
  }
  edges[newHead].saliency = edges[newHead].iLine * newD;
  edges[newHead].lineLength = newLength;
}

int FeatureConvincedDenoising::traverseLineToOppositeEnd(
    vx::Array2<float> vertices, int start, int awayFromNode) {
  if (awayFromNode == edges.at(start).vertexOne) {
    if (edges.at(start).lineNeighbourTwo >= 0) {
      return traverseLineToOppositeEnd(vertices,
                                       edges.at(start).lineNeighbourTwo,
                                       edges.at(start).vertexTwo);
    }
  }
  if (awayFromNode == edges.at(start).vertexTwo) {
    if (edges.at(start).lineNeighbourOne >= 0) {
      return traverseLineToOppositeEnd(vertices,
                                       edges.at(start).lineNeighbourOne,
                                       edges.at(start).vertexOne);
    }
  }
  return start;
}

void FeatureConvincedDenoising::setParameters(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    laplacians.append(QVector3D(0, 0, 0));
    lapNeighbours.append(0);
  }
  double mselNoisy = 0;

  computeNoise(vertices, triangles, mselNoisy);
  meanLength = sqrt(mselNoisy);

  FastEffectiveDPFilter filter;
  filter.compute(vertices, triangles, 0.4, 4, prog);

  double mselClean = 0;

  computeNoise(vertices, triangles, mselClean);

  double noise =
      sqrt((mselNoisy * mselNoisy) / (2 * mselClean * mselClean) - 0.5);

  resetVertices(vertices);

  if (noise < 0.1) {
    taubinIterations = 0;
    fedIterations = 5;
    minFeatureEdgeAngle = 0.75;
    bilateralFilterIterations = 3;
    filterIterations = 3;
  } else if (noise < 0.25) {
    taubinIterations = 1;
    fedIterations = 8;
    minFeatureEdgeAngle = 0.85;
    bilateralFilterIterations = 4;
    filterIterations = 4;
  } else if (noise < 0.45) {
    taubinIterations = 2;
    fedIterations = 10;
    minFeatureEdgeAngle = 0.82;
    bilateralFilterIterations = 6;
    filterIterations = 6;
  } else {
    taubinIterations = 2;
    fedIterations = 12;
    minFeatureEdgeAngle = 0.78;
    bilateralFilterIterations = 8;
    filterIterations = 8;
  }

  lineAngleThreshold = 2.5;        // experiment
  sigmaSpacial = sqrt(mselClean);  //??
  sigmaSignal = 0.35;
  maxNeighbourhoodSize = 12;
  minimumLineLenghth = 2;
  extensionAngle = lineAngleThreshold;
}

void FeatureConvincedDenoising::computeNoise(
    vx::Array2<float> vertices, vx::Array2<const uint32_t> triangles,
    double& msel) {
  for (int i = 0; i < trianglesCopy.size(); i++) {
    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));

    double edgeLenght = (first - second).length();
    edgeLenght = edgeLenght * edgeLenght;
    msel += edgeLenght;

    edgeLenght = (first - third).length();
    edgeLenght = edgeLenght * edgeLenght;
    msel += edgeLenght;

    edgeLenght = (second - third).length();
    edgeLenght = edgeLenght * edgeLenght;
    msel += edgeLenght;

    laplacians[triangles(i, 0)] += (second - first) + (third - first);
    laplacians[triangles(i, 1)] += (first - second) + (third - second);
    laplacians[triangles(i, 2)] += (first - third) + (second - third);

    lapNeighbours[triangles(i, 0)] += 2;
    lapNeighbours[triangles(i, 1)] += 2;
    lapNeighbours[triangles(i, 2)] += 2;
  }

  msel = msel / (trianglesCopy.size() * 3);

  for (int i = 0; i < laplacians.length(); i++) {
    if (lapNeighbours.at(i) > 0) {
      meanLaplacian += laplacians.at(i).length() / lapNeighbours.at(i);
    }
  }
  meanLaplacian /= laplacians.size();
}

void FeatureConvincedDenoising::resetVertices(vx::Array2<float>& vertices) {
  for (int i = 0; i < verticesCopy.size(); i++) {
    vertices(i, 0) = verticesCopy.at(i).x();
    vertices(i, 1) = verticesCopy.at(i).y();
    vertices(i, 2) = verticesCopy.at(i).z();
  }
}
