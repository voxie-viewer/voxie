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

#ifndef EDGE_H
#define EDGE_H
#include <QHash>

class Edge {
 public:
  Edge();
  int triangleOne, triangleTwo, vertexOne, vertexTwo;
  double angleCosine;
  bool feature;
  bool extension;
  int index;

  // information about the feature line this edge is part of
  int lineNeighbourOne;  // haengt an vertexOne
  int lineNeighbourTwo;  // haengt an vertexTwo
  int lineParent;
  double saliency;
  double directionDiffOne;
  double directionDiffTwo;
  int lineLength;
  double dThisEdge;
  double iLine;  // beschreibt das durchschnittliche i der gesamten Line (nur
                 // wichtig für Koepfe)
  double dLine;

  int followToLineHead();
};
uint qHash(const Edge& edge);
bool operator==(const Edge& other, const Edge& th);
#endif  // EDGE_H
