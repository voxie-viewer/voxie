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

#ifndef CELL_H
#define CELL_H

#include <math.h>

#include <QVector3D>
#include <vector>

/**
 * @brief Used to partition the space into subcells.
 */
class Cell {
  float startX, endX, startY, endY, startZ, endZ;

  std::vector<QVector3D> vertices;

 public:
  // getter
  float getStartX() { return startX; }
  float getEndX() { return endX; }
  float getStartY() { return startY; }
  float getEndY() { return endY; }
  float getStartZ() { return startZ; }
  float getEndZ() { return endZ; }

  float getEdgeLength() { return endX - startX; }
  double getDiagonal() {
    return sqrt((pow(getEndX() - getStartX(), 2)) +
                (pow(getEndY() - getStartY(), 2)) +
                (pow(getEndZ() - getStartZ(), 2)));
  }
  std::vector<QVector3D> getVertices() { return vertices; }

  // setter
  void setStartX(float x);
  void setEndX(float x);
  void setStartY(float y);
  void setEndY(float y);
  void setStartZ(float z);
  void setEndZ(float z);

  /**
   * set start- and endvalues for given Cell and thereby create a vector
   * containing intersecting vertices
   *
   * @brief Cell::setValues
   * @param x1 start-value on x-axis
   * @param x2 end-value on x-axis
   * @param y1 start-value on y-axis
   * @param y2 end-value on y-axis
   * @param z1 start-value on z-axis
   * @param z2 end-value on z-axis
   */
  void setValues(float x1, float x2, float y1, float y2, float z1, float z2);

  /*
   * add the given QVector3D to the list of intersecting vertices
   *
   * @param vertex the QVector3D with the coordinates of the vertex
   */
  void addVertexToCell(QVector3D vertex);
};

#endif  // CELL_H
