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

#include "VerifySurface.hpp"
#include <QString>
#include <cmath>
#include <map>
#include <utility>

VerifySurface::VerifySurface(vx::Array2<const uint32_t> triangles,
                             vx::Array2<const float> vertices)
    : triangles(triangles), vertices(vertices) {}

void VerifySurface::run(
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        op) {
  // Go through each triangles edges and increment/decrement a value dependent
  // of its direction
  std::map<std::pair<uint32_t, uint32_t>, int32_t> edgeCounts;
  for (size_t index = 0; index < triangles.size<0>(); index++) {
    for (size_t vertex = 0; vertex < 3; vertex++) {
      size_t nextVertex = (vertex + 1) % 3;

      if (!std::isfinite(vertices(triangles(index, vertex), 0)) ||
          !std::isfinite(vertices(triangles(index, vertex), 1)) ||
          !std::isfinite(vertices(triangles(index, vertex), 2)) ||
          !std::isfinite(vertices(triangles(index, nextVertex), 0)) ||
          !std::isfinite(vertices(triangles(index, nextVertex), 1)) ||
          !std::isfinite(vertices(triangles(index, nextVertex), 2))) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            QString("Edge between (%1,%2,%3) and (%4,%5,%6) has "
                    "at least one non-finite value")
                .arg(vertices(triangles(index, vertex), 0))
                .arg(vertices(triangles(index, vertex), 1))
                .arg(vertices(triangles(index, vertex), 2))
                .arg(vertices(triangles(index, nextVertex), 0))
                .arg(vertices(triangles(index, nextVertex), 1))
                .arg(vertices(triangles(index, nextVertex), 2)));
      }

      if (triangles(index, vertex) <= triangles(index, nextVertex)) {
        edgeCounts[std::make_pair(triangles(index, vertex),
                                  triangles(index, nextVertex))] += 1;
      } else {
        edgeCounts[std::make_pair(triangles(index, nextVertex),
                                  triangles(index, vertex))] -= 1;
      }
    }

    // Progressbar (only update every percent)
    if (index % (2 * triangles.size<0>() / 50) == 0) {
      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
          static_cast<double>(index) /
              static_cast<double>(2 * triangles.size<0>()),
          vx::emptyOptions()));
    }
  }

  // Check that there are equal amounts of edges in both directions which means
  // the map value has to be zero
  uint32_t count = 0;
  for (std::pair<const std::pair<uint32_t, uint32_t>, int32_t> edgeCount :
       edgeCounts) {
    if (edgeCount.second != 0) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          QString("Edge between (%1,%2,%3) and (%4,%5,%6) has "
                                  "an unequal amount of directional edges")
                              .arg(vertices(edgeCount.first.first, 0))
                              .arg(vertices(edgeCount.first.first, 1))
                              .arg(vertices(edgeCount.first.first, 2))
                              .arg(vertices(edgeCount.first.second, 0))
                              .arg(vertices(edgeCount.first.second, 1))
                              .arg(vertices(edgeCount.first.second, 2)));
    }

    // Progressbar (only update every percent)
    if (count++ % (2 * edgeCounts.size() / 50) == 0) {
      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
          0.5 + static_cast<double>(count) /
                    static_cast<double>(2 * edgeCounts.size()),
          vx::emptyOptions()));
    }
  }
}
