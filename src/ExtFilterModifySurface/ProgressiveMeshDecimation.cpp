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

#include "ProgressiveMeshDecimation.hpp"

#include <math.h>

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

ProgressiveMeshDecimation::ProgressiveMeshDecimation() {}

void ProgressiveMeshDecimation::compute(
    vx::Array2<const float> vertices, vx::Array2<const uint32_t> triangles,
    double percentage, double angle,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  trianglesEliminated = 0;
  verticesEliminated = 0;
  featureThreshold = angle;
  int targetEliminations = vertices.size<0>() * percentage;

  copyTriangles(triangles);
  copyVertices(vertices);
  setUpData(vertices, triangles);
  qDebug() << "set up complete";
  classifyVertices(triangles);
  qDebug() << "classification complete";
  calculateErrors(triangles);
  qDebug() << "errors calculated";
  decimate(targetEliminations, triangles, prog);

  mapVertices(vertices);

  qDebug() << trianglesEliminated;
  qDebug() << verticesEliminated;
}

// insert mappings of each vertex!
void ProgressiveMeshDecimation::getTrianglesResults(
    vx::Array2<uint32_t>& triangles) {
  int counter = 0;
  for (int i = 0; i < trianglesCopy.size(); i++) {
    if (stillExistsTriangle.at(i)) {
      triangles(counter, 0) =
          verticesIndexMapping.value(trianglesCopy.at(i).at(0));
      triangles(counter, 1) =
          verticesIndexMapping.value(trianglesCopy.at(i).at(1));
      triangles(counter, 2) =
          verticesIndexMapping.value(trianglesCopy.at(i).at(2));
      counter++;
    }
  }
}

void ProgressiveMeshDecimation::getVerticesResults(
    vx::Array2<float>& vertices) {
  int counter = 0;
  for (int i = 0; i < verticesCopy.size(); i++) {
    if (stillExistsVertex.at(i)) {
      vertices(counter, 0) = verticesCopy.at(i).at(0);
      vertices(counter, 1) = verticesCopy.at(i).at(1);
      vertices(counter, 2) = verticesCopy.at(i).at(2);
      counter++;
    }
  }
}

void ProgressiveMeshDecimation::copyResults(vx::Array2<const float> vertices,
                                            vx::Array2<uint32_t>& triangles) {
  Q_UNUSED(vertices);  // TODO

  for (int i = 0; i < trianglesCopy.size(); i++) {
    triangles(i, 0) = trianglesCopy.at(i).at(0);
    triangles(i, 1) = trianglesCopy.at(i).at(1);
    triangles(i, 2) = trianglesCopy.at(i).at(2);
  }
}

void ProgressiveMeshDecimation::decimate(
    int threshold, vx::Array2<const uint32_t> triangles,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  while (verticesEliminated < threshold) {
    if (priorityQueue.isEmpty()) {
      break;
    }
    int first = priorityQueue.first();
    removeTopFromQueue();
    eliminate(first, triangles);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (double)verticesEliminated / (double)threshold, vx::emptyOptions()));
  }
}

void ProgressiveMeshDecimation::eliminate(
    int index, vx::Array2<const uint32_t> triangles) {
  QList<int> neighbourSet;
  QList<int> candidates;
  QVector<int> neighbours;

  // Build neighbourhood and candidates for an edge collapse
  for (int i = 0; i < neighbourTriangles.at(index).size(); i++) {
    int triangle = neighbourTriangles.at(index).at(i);
    neighbours.append(trianglesCopy.at(triangle).at(0));
    neighbours.append(trianglesCopy.at(triangle).at(1));
    neighbours.append(trianglesCopy.at(triangle).at(2));
  }
  while (!neighbours.isEmpty()) {
    int n = neighbours.first();
    if (isBoundary[index]) {
      if ((neighbours.count(n) == 1) && (n != index)) {
        candidates.append(n);
      }
    } else if (isSimple[index] && n != index) {
      candidates.append(n);
    }
    if (n != index) {
      neighbourSet.append(n);
    }
    neighbours.removeAll(n);
  }

  if (isSimple[index]) {
    collapseSimple(index, candidates, triangles);
  }
  if (isBoundary[index]) {
    collapseBoundary(index, candidates, neighbourSet, triangles);
  }
}

void ProgressiveMeshDecimation::collapseSimple(
    int index, QList<int> candidates, vx::Array2<const uint32_t> triangles) {
  QList<int> candidateTriangleNeighbour;
  QList<bool> isFeature;
  bool containsFeatures = false;
  for (int i = 0; i < candidates.length(); i++) {
    candidateTriangleNeighbour.append(-1);
    isFeature.append(false);
  }

  for (int i = 0; i < neighbourTriangles.at(index).size(); i++) {
    int one = trianglesCopy.at(neighbourTriangles.at(index).at(i)).at(0);
    int two = trianglesCopy.at(neighbourTriangles.at(index).at(i)).at(1);
    int three = trianglesCopy.at(neighbourTriangles.at(index).at(i)).at(2);

    int candidatesIndexOne = candidates.indexOf(one);
    int candidatesIndexTwo = candidates.indexOf(two);
    int candidatesIndexThree = candidates.indexOf(three);

    if (candidatesIndexOne >= 0 &&
        candidateTriangleNeighbour.at(candidatesIndexOne) == -1) {
      candidateTriangleNeighbour[candidatesIndexOne] =
          neighbourTriangles.at(index).at(i);
    } else if (candidatesIndexOne >= 0) {
      isFeature[candidatesIndexOne] =
          QVector3D().dotProduct(
              calculateNormal(candidates.at(candidatesIndexOne), triangles),
              calculateNormal(candidateTriangleNeighbour.at(candidatesIndexOne),
                              triangles)) > featureThreshold;
      if (isFeature[candidatesIndexOne]) {
        containsFeatures = true;
      }
    }
    if (candidatesIndexTwo >= 0 &&
        candidateTriangleNeighbour.at(candidatesIndexTwo) == -1) {
      candidateTriangleNeighbour[candidatesIndexTwo] =
          neighbourTriangles.at(index).at(i);
    } else if (candidatesIndexTwo >= 0) {
      isFeature[candidatesIndexTwo] =
          QVector3D().dotProduct(
              calculateNormal(candidates.at(candidatesIndexTwo), triangles),
              calculateNormal(candidateTriangleNeighbour.at(candidatesIndexTwo),
                              triangles)) > featureThreshold;
      if (isFeature[candidatesIndexTwo]) {
        containsFeatures = true;
      }
    }
    if (candidatesIndexThree >= 0 &&
        candidateTriangleNeighbour.at(candidatesIndexThree) == -1) {
      candidateTriangleNeighbour[candidatesIndexThree] =
          neighbourTriangles.at(index).at(i);
    } else if (candidatesIndexThree >= 0) {
      isFeature[candidatesIndexThree] =
          QVector3D().dotProduct(
              calculateNormal(candidates.at(candidatesIndexThree), triangles),
              calculateNormal(
                  candidateTriangleNeighbour.at(candidatesIndexThree),
                  triangles)) > featureThreshold;
      if (isFeature[candidatesIndexThree]) {
        containsFeatures = true;
      }
    }
  }

  double minLength = -1;
  int indexBestCandidate = -1;
  for (int i = 0; i < candidates.length(); i++) {
    if ((!containsFeatures || isFeature[i]) &&
        isValidTriangulation(candidates.at(i), index, candidates, triangles)) {
      double distance =
          (positions.at(index) - positions.at(candidates.at(i))).length();
      distance = abs(distance);
      if (minLength < 0 || distance < minLength) {
        minLength = distance;
        indexBestCandidate = candidates.at(i);
      }
    }
  }
  if (indexBestCandidate < 0) {
    //        errors[index] = errors[index] * 2;
    //        insertIntoQueue(index, priorityQueue.length());
    return;
  }
  edgeCollapse(index, indexBestCandidate, candidates, triangles);
}

void ProgressiveMeshDecimation::collapseBoundary(
    int index, QList<int> candidates, QList<int> neighbours,
    vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(neighbours);  // TODO

  QList<int> candidateTriangleNeighbour;

  double minLength = -1;
  int indexBestCandidate = -1;

  if (candidates.size() == 1) {
    indexBestCandidate = candidates.at(0);
  }

  for (int i = 0; i < candidates.length(); i++) {
    if (candidates.size() == 1) {
      continue;
    }
    if (isValidTriangulation(candidates.at(i), index, candidates, triangles)) {
      double distance =
          (positions.at(index) - positions.at(candidates.at(i))).length();
      distance = abs(distance);
      if (minLength < 0 || distance < minLength) {
        minLength = distance;
        indexBestCandidate = candidates.at(i);
      }
    }
  }
  if (indexBestCandidate < 0) {
    return;
  }
  edgeCollapse(index, indexBestCandidate, candidates, triangles);
}

void ProgressiveMeshDecimation::edgeCollapse(
    int fromVertex, int toVertex, QList<int> neighbours,
    vx::Array2<const uint32_t> triangles) {
  stillExistsVertex[fromVertex] = false;
  verticesEliminated++;

  QList<int> neighbourTri = neighbourTriangles.at(fromVertex);

  QList<int> toEliminate;

  for (int i = 0; i < neighbourTriangles.at(fromVertex).size(); i++) {
    int currentTriangle = neighbourTri.at(i);
    QList<int> indices;

    indices.append(trianglesCopy.at(currentTriangle).at(0));
    indices.append(trianglesCopy.at(currentTriangle).at(1));
    indices.append(trianglesCopy.at(currentTriangle).at(2));

    if (indices.contains(fromVertex) && indices.contains(toVertex)) {
      stillExistsTriangle[currentTriangle] = false;
      trianglesCopy[currentTriangle][0] = 0;
      trianglesCopy[currentTriangle][1] = 0;
      trianglesCopy[currentTriangle][2] = 0;

      trianglesEliminated++;

      for (int j = 0; j < 3; j++) {
        int vertex = indices.at(j);
        if (vertex == fromVertex) {
          toEliminate.append(currentTriangle);
        } else {
          neighbourTriangles[vertex].removeAll(currentTriangle);
        }
      }
    } else {
      int ind = indices.indexOf(fromVertex);
      trianglesCopy[currentTriangle][ind] = toVertex;
      neighbourTriangles[toVertex].append(currentTriangle);
    }
  }

  for (int i = 0; i < toEliminate.size(); i++) {
    neighbourTriangles[fromVertex].removeAll(toEliminate.at(i));
  }

  for (int i = 0; i < neighbours.size(); i++) {
    if (!isNonManifold.at(neighbours.at(i))) {
      deleteFromQueue(indicesInQueue.at(neighbours.at(i)));
    }
    classify(neighbours.at(i), triangles);
    errors[neighbours.at(i)] =
        errors.at(neighbours.at(i)) + errors.at(fromVertex);
    if (!isNonManifold.at(neighbours.at(i))) {
      insertIntoQueue(neighbours.at(i), priorityQueue.length());
    }
  }
}

bool ProgressiveMeshDecimation::isValidTriangulation(
    int indexCandidate, int toBeCollapsed, QList<int> neighbourSet,
    vx::Array2<const uint32_t> triangles) {
  bool valid = true;
  QList<QList<int>> loopNeighbours;

  // build loop ring around vertex to be collapsed
  for (int i = 0; i < neighbourSet.size(); i++) {
    loopNeighbours.append(QList<int>());
  }
  if (loopNeighbours.size() == 2) {
    return true;
  }

  for (int i = 0; i < neighbourTriangles.at(toBeCollapsed).size(); i++) {
    QList<int> indices;
    indices.append(
        trianglesCopy.at(neighbourTriangles.at(toBeCollapsed).at(i)).at(0));
    indices.append(
        trianglesCopy.at(neighbourTriangles.at(toBeCollapsed).at(i)).at(1));
    indices.append(
        trianglesCopy.at(neighbourTriangles.at(toBeCollapsed).at(i)).at(2));
    indices.removeAll(toBeCollapsed);

    loopNeighbours[neighbourSet.indexOf(indices.at(0))].append(indices.at(1));
    if (loopNeighbours.at(neighbourSet.indexOf(indices.at(0))).size() > 2) {
      return false;
    }
    loopNeighbours[neighbourSet.indexOf(indices.at(1))].append(indices.at(0));
    if (loopNeighbours.at(neighbourSet.indexOf(indices.at(1))).size() > 2) {
      return false;
    }
  }

  // check for validity through cutting planes
  for (int i = 0; i < neighbourSet.size(); i++) {
    if (neighbourSet.at(i) == indexCandidate) {
      continue;
    }
    if (loopNeighbours.at(i).contains(indexCandidate)) {
      continue;
    }
    QList<int> subLoopOne;
    QList<int> subLoopTwo;

    subLoopOne.append(
        loopNeighbours.at(neighbourSet.indexOf(indexCandidate)).at(0));
    subLoopTwo.append(
        loopNeighbours.at(neighbourSet.indexOf(indexCandidate)).at(1));

    buildSubLoop(indexCandidate, neighbourSet.at(i),
                 loopNeighbours.at(neighbourSet.indexOf(indexCandidate)).at(0),
                 neighbourSet, loopNeighbours, subLoopOne);
    buildSubLoop(indexCandidate, neighbourSet.at(i),
                 loopNeighbours.at(neighbourSet.indexOf(indexCandidate)).at(1),
                 neighbourSet, loopNeighbours, subLoopTwo);

    QVector3D normal(0, 0, 0);
    double areaSum = 0;
    for (int j = 0; j < neighbourTriangles.at(toBeCollapsed).size(); j++) {
      int triangle = neighbourTriangles.at(toBeCollapsed).at(j);

      double area = calculateTriangleArea(triangle, triangles);
      normal = normal + calculateNormal(triangle, triangles) * area;

      areaSum += area;
    }
    normal = normal / areaSum;
    normal = normal * 50000;
    normal.normalize();

    QVector3D dir =
        (positions.at(indexCandidate) - positions.at(neighbourSet.at(i)));

    bool abovePlaneOne = true;
    bool abovePlaneTwo = true;
    if (!allOnSameSide(subLoopOne, dir, normal, indexCandidate,
                       abovePlaneOne)) {
      return false;
    }
    if (!allOnSameSide(subLoopTwo, dir, normal, indexCandidate,
                       abovePlaneTwo)) {
      return false;
    }
    if (abovePlaneOne && abovePlaneTwo) {
      return false;
    }
    if (!abovePlaneOne && !abovePlaneTwo) {
      return false;
    }
  }

  return valid;
}

bool ProgressiveMeshDecimation::allOnSameSide(QList<int> vertices,
                                              QVector3D dirOne,
                                              QVector3D dirTwo,
                                              int vertexOnPlane,
                                              bool& abovePlane) {
  // bool sameSide = true; // TODO: ?
  double lastDistance = 0;
  for (int i = 0; i < vertices.length(); i++) {
    double distance = positions.at(vertices.at(i))
                          .distanceToPlane(positions.at(vertexOnPlane),
                                           QVector3D().normal((500 * dirOne),
                                                              (500 * dirTwo)));

    if (lastDistance * distance < 0) {
      return false;
    }
    abovePlane = (distance > 0);
    lastDistance = distance;
  }
  return true;
}

void ProgressiveMeshDecimation::buildSubLoop(int ignoreOne, int ignoreTwo,
                                             int current, QList<int> neighbours,
                                             QList<QList<int>> loopNeighbours,
                                             QList<int>& subLoop) {
  int neighbourIndex = neighbours.indexOf(current);

  if (loopNeighbours.at(neighbourIndex).size() == 4) {
    for (int i = 0; i < loopNeighbours.at(neighbourIndex).size(); i++) {
    }
  }
  for (int i = 0; i < loopNeighbours.at(neighbourIndex).size(); i++) {
    int neighbour = loopNeighbours.at(neighbourIndex).at(i);
    if (neighbour != ignoreOne && neighbour != ignoreTwo) {
      subLoop.append(neighbour);
      buildSubLoop(current, ignoreTwo, neighbour, neighbours, loopNeighbours,
                   subLoop);
    }
  }
}

void ProgressiveMeshDecimation::classifyVertices(
    vx::Array2<const uint32_t> triangles) {
  for (int i = 0; i < positions.size(); i++) {
    classify(i, triangles);
  }
}

void ProgressiveMeshDecimation::calculateErrors(
    vx::Array2<const uint32_t> triangles) {
  for (int i = 0; i < positions.length(); i++) {
    errors[i] = calculateError(i, triangles);
    if (!isNonManifold.at(i)) {
      insertIntoQueue(i, priorityQueue.length());
    }
  }
}

double ProgressiveMeshDecimation::calculateError(
    int index, vx::Array2<const uint32_t> triangles) {
  double error = 0;
  if (isSimple.at(index)) {
    // distance to average plane
    QVector3D normal(0, 0, 0);
    QVector3D centroid(0, 0, 0);
    double areaSum = 0;
    for (int i = 0; i < neighbourTriangles.at(index).size(); i++) {
      int triangle = neighbourTriangles.at(index).at(i);

      double area = calculateTriangleArea(triangle, triangles);
      normal = normal + calculateNormal(triangle, triangles) * area;
      centroid = centroid + calculateCentroid(triangle, triangles) * area;

      areaSum += area;
    }
    normal = normal / areaSum;
    normal.normalize();
    centroid = centroid / areaSum;

    error = QVector3D().dotProduct(normal, (positions.at(index) - centroid));
    error = abs(error);
  }

  else if (isBoundary.at(index)) {
    // special case
    if (neighbourTriangles.at(index).length() == 1) {
      error = sqrt(calculateTriangleArea(neighbourTriangles.at(index).first(),
                                         triangles));
    }
    // distance to line
    else {
      QList<int> lineElements;
      QVector<int> neighbours;
      for (int i = 0; i < neighbourTriangles.at(index).size(); i++) {
        int triangle = neighbourTriangles.at(index).at(i);
        neighbours.append(triangles(triangle, 0));
        neighbours.append(triangles(triangle, 1));
        neighbours.append(triangles(triangle, 2));
      }
      while (!neighbours.isEmpty()) {
        int n = neighbours.first();
        if (neighbours.count(n) == 1) {
          lineElements.append(n);
        }
        neighbours.removeAll(n);
      }

      if (lineElements.size() > 2) {
        qDebug() << "found nonmanifold as boundary";
      }

      error = positions.at(index).distanceToLine(
          positions.at(lineElements.first()),
          (positions.at(lineElements.first()) -
           positions.at(lineElements.last())));
    }
  } else if (isNonManifold.at(index)) {
    // shall not be eliminated
  } else {
    qDebug() << "Vertex has no classification!";
  }

  return error;
}

void ProgressiveMeshDecimation::classify(int index,
                                         vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(triangles);  // TODO?

  QVector<int> neighbours;
  bool simple = true;

  for (int i = 0; i < neighbourTriangles.at(index).size(); i++) {
    int triangle = neighbourTriangles.at(index).at(i);
    neighbours.append(trianglesCopy.at(triangle).at(0));
    neighbours.append(trianglesCopy.at(triangle).at(1));
    neighbours.append(trianglesCopy.at(triangle).at(2));
  }
  while (!neighbours.isEmpty()) {
    int n = neighbours.first();
    if ((n != index) && (neighbours.count(n) != 2)) {
      simple = false;
      if (neighbours.count(n) > 2) {
        isNonManifold[index] = true;
      }
    }
    neighbours.removeAll(n);
  }
  isSimple[index] = simple;
  if (!simple && !isNonManifold[index]) {
    isBoundary[index] = true;
  }
}

void ProgressiveMeshDecimation::setUpData(
    vx::Array2<const float> vertices, vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    QVector3D vec(vertices(i, 0), vertices(i, 1), vertices(i, 2));
    positions.append(vec);
    indicesInQueue.append(-1);
    neighbourTriangles.append(QList<int>());
    errors.append(0.0);
    isNonManifold.append(false);
    isBoundary.append(false);
    isSimple.append(false);
    stillExistsVertex.append(true);
  }

  for (size_t i = 0; i < triangles.size<0>(); i++) {
    int one = trianglesCopy.at(i).at(0);
    int two = trianglesCopy.at(i).at(1);
    int three = trianglesCopy.at(i).at(2);

    neighbourTriangles[one].append(i);
    neighbourTriangles[two].append(i);
    neighbourTriangles[three].append(i);
    stillExistsTriangle.append(true);
  }
}

int ProgressiveMeshDecimation::insertIntoQueue(int index, int pos) {
  int currentIndex = pos;
  int parentIndex = 0;

  priorityQueue.append(index);

  indicesInQueue[index] = pos;

  while (currentIndex != 0) {
    parentIndex = currentIndex;
    if (parentIndex % 2 == 0) {
      parentIndex--;
    }
    parentIndex--;
    parentIndex = parentIndex / 2;

    if (errors.at(index) > errors.at(priorityQueue.at(parentIndex))) {
      priorityQueue[currentIndex] = index;
      indicesInQueue[index] = currentIndex;
      break;
    } else {
      priorityQueue[currentIndex] = priorityQueue.at(parentIndex);
      indicesInQueue[priorityQueue.at(parentIndex)] = currentIndex;
      priorityQueue[parentIndex] = index;
      indicesInQueue[index] = parentIndex;
      currentIndex = parentIndex;
    }
  }
  return currentIndex;
}

void ProgressiveMeshDecimation::deleteFromQueue(int index) {
  int currentIndex = index;
  if (index == -1) {
    return;
  }
  indicesInQueue[priorityQueue.at(index)] = -1;

  while (true) {
    int leftChild = currentIndex * 2 + 1;
    int rightChild = currentIndex * 2 + 2;

    // hat zwei Kindkknoten
    if (rightChild < priorityQueue.length()) {
      if (errors.at(priorityQueue.at(leftChild)) <
          errors.at(priorityQueue.at(rightChild))) {
        priorityQueue[currentIndex] = priorityQueue.at(leftChild);
        indicesInQueue[priorityQueue.at(leftChild)] = currentIndex;
        currentIndex = leftChild;
      } else {
        priorityQueue[currentIndex] = priorityQueue.at(rightChild);
        indicesInQueue[priorityQueue.at(rightChild)] = currentIndex;
        currentIndex = rightChild;
      }
    }  // hat genau einen Kindknoten
    else if (leftChild < priorityQueue.length()) {
      priorityQueue[currentIndex] = priorityQueue.at(leftChild);
      indicesInQueue[priorityQueue.at(leftChild)] = currentIndex;
      priorityQueue.removeLast();  // ist leftChild
      break;
    }  // hat keinen Kindknoten
    else {
      if (currentIndex < priorityQueue.length() - 1) {
        priorityQueue[currentIndex] = priorityQueue.last();
        indicesInQueue[priorityQueue.last()] = currentIndex;
      }
      priorityQueue.removeLast();
      break;
    }
  }
}

void ProgressiveMeshDecimation::removeTopFromQueue() { deleteFromQueue(0); }

double ProgressiveMeshDecimation::calculateTriangleArea(
    int i, vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(triangles);  // TODO?

  QVector3D one = positions.at(trianglesCopy.at(i).at(0));
  QVector3D two = positions.at(trianglesCopy.at(i).at(1));
  QVector3D three = positions.at(trianglesCopy.at(i).at(2));

  QVector3D direction = one - two;
  direction.normalize();

  float area =
      (one - two).length() * three.distanceToLine(one, direction) / 2.0;
  return area;
}

QVector3D ProgressiveMeshDecimation::calculateCentroid(
    int i, vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(triangles);  // TODO?

  QVector3D one = positions.at(trianglesCopy.at(i).at(0));
  QVector3D two = positions.at(trianglesCopy.at(i).at(1));
  QVector3D three = positions.at(trianglesCopy.at(i).at(2));

  QVector3D centroid = (one + two + three) / 3;

  return centroid;
}

QVector3D ProgressiveMeshDecimation::calculateNormal(
    int i, vx::Array2<const uint32_t> triangles) {
  Q_UNUSED(triangles);  // TODO?

  QVector3D one = positions.at(trianglesCopy.at(i).at(0));
  QVector3D two = positions.at(trianglesCopy.at(i).at(1));
  QVector3D three = positions.at(trianglesCopy.at(i).at(2));

  QVector3D normal = QVector3D().normal(one * 5000, two * 5000, three * 5000);
  normal.normalize();
  return normal;
}

void ProgressiveMeshDecimation::copyTriangles(
    vx::Array2<const uint32_t> triangles) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    trianglesCopy.append(QList<int>());
    trianglesCopy[trianglesCopy.size() - 1].append(triangles(i, 0));
    trianglesCopy[trianglesCopy.size() - 1].append(triangles(i, 1));
    trianglesCopy[trianglesCopy.size() - 1].append(triangles(i, 2));
  }
}

void ProgressiveMeshDecimation::copyVertices(vx::Array2<const float> vertices) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    verticesCopy.append(QList<float>());
    verticesCopy[i].append(vertices(i, 0));
    verticesCopy[i].append(vertices(i, 1));
    verticesCopy[i].append(vertices(i, 2));
  }
}

void ProgressiveMeshDecimation::mapVertices(vx::Array2<const float> vertices) {
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    if (stillExistsVertex.at(i)) {
      verticesIndexMapping.insert(i, verticesIndexMapping.size());
    }
  }
}

int ProgressiveMeshDecimation::getRemainingVertexCount() {
  return verticesIndexMapping.size();
}

int ProgressiveMeshDecimation::getRemainingTriangleCount() {
  int counter = 0;
  for (int i = 0; i < stillExistsTriangle.size(); i++) {
    if (stillExistsTriangle.at(i)) {
      counter++;
    }
  }
  return counter;
}
