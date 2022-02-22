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

#include "TaubinFiltering.hpp"

// TODO: Get rid of QtGui?
#include <QtGui/QVector3D>

TaubinFiltering::TaubinFiltering(int it, double att) {
  iterations = it;
  attenuationFactor = att;
  inflationFactor = -(att * 1.06);
}

void TaubinFiltering::compute(
    vx::Array2<float>& vertices, vx::Array2<const uint32_t> triangles,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  for (int i = 0; i < 2 * iterations; i++) {
    QList<QVector3D> difference;
    QList<int> neighbours;
    for (size_t j = 0; j < vertices.size<0>(); j++) {
      difference.append(QVector3D());
      neighbours.append(0);
    }
    double scaling;
    if (i % 2 == 0) {
      scaling = attenuationFactor;
    } else {
      scaling = inflationFactor;
    }
    computeDiffs(vertices, triangles, difference, neighbours, scaling);
    HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(
        (double)i / (2 * iterations), vx::emptyOptions()));
  }
}

void TaubinFiltering::computeDiffs(vx::Array2<float>& vertices,
                                   vx::Array2<const uint32_t> triangles,
                                   QList<QVector3D>& difference,
                                   QList<int>& neighbours, double scaling) {
  for (size_t i = 0; i < triangles.size<0>(); i++) {
    auto idxO = triangles(i, 0);
    auto idxTw = triangles(i, 1);
    auto idxTh = triangles(i, 2);

    neighbours[idxO] += 2;
    neighbours[idxTw] += 2;
    neighbours[idxTh] += 2;

    QVector3D first(vertices(triangles(i, 0), 0), vertices(triangles(i, 0), 1),
                    vertices(triangles(i, 0), 2));
    QVector3D second(vertices(triangles(i, 1), 0), vertices(triangles(i, 1), 1),
                     vertices(triangles(i, 1), 2));
    QVector3D third(vertices(triangles(i, 2), 0), vertices(triangles(i, 2), 1),
                    vertices(triangles(i, 2), 2));
    difference[idxO] += ((second - first) + (third - first)) * scaling;
    difference[idxTw] += ((first - second) + (third - second)) * scaling;
    difference[idxTh] += ((first - third) + (second - third)) * scaling;
  }
  for (size_t i = 0; i < vertices.size<0>(); i++) {
    vertices(i, 0) = vertices(i, 0) + difference[i].x() / neighbours[i];
    vertices(i, 1) = vertices(i, 1) + difference[i].y() / neighbours[i];
    vertices(i, 2) = vertices(i, 2) + difference[i].z() / neighbours[i];
  }
}
