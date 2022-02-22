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

#include "AddNormalAttribute.hpp"

#include <QtGui/QVector3D>

#include <cmath>

#define PI 3.14159265

AddNormalAttribute::AddNormalAttribute() {}

void AddNormalAttribute::compute(
    vx::Array2<const float>& vertices, vx::Array2<const uint>& triangles,
    vx::Array2<float>& normals,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  // using triangles
  for (uint32_t i = 0; i < triangles.size<0>(); i++) {
    uint aInd = triangles(i, 0);
    uint bInd = triangles(i, 1);
    uint cInd = triangles(i, 2);
    QVector3D a(vertices(aInd, 0), vertices(aInd, 1), vertices(aInd, 2));
    QVector3D b(vertices(bInd, 0), vertices(bInd, 1), vertices(bInd, 2));
    QVector3D c(vertices(cInd, 0), vertices(cInd, 1), vertices(cInd, 2));
    QVector3D normal = QVector3D::crossProduct(b - c, c - a);
    for (int coord = 0; coord < 3; coord++) {
      normals(aInd, coord) += normal[coord];
      normals(bInd, coord) += normal[coord];
      normals(cInd, coord) += normal[coord];
    }
  }
  for (uint32_t i = 0; i < normals.size<0>(); i++) {
    double len = 0;
    for (int coord = 0; coord < 3; coord++) {
      len += normals(i, coord) * normals(i, coord);
    }
    if (len != 0) {
      len = std::sqrt(len);
      for (int coord = 0; coord < 3; coord++) {
        normals(i, coord) /= len;
      }
    }
  }

  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(1.00, vx::emptyOptions()));
}

void AddNormalAttribute::compute(
    vx::Array2<const float>& vertices, vx::Array3<const float>& volume,
    QVector3D& spacing, QVector3D& origin, vx::Array2<float>& normals,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        prog) {
  // using volume
  int nx = volume.size<0>();
  int ny = volume.size<1>();
  int nz = volume.size<2>();
  float* dx = new float[nx * ny * nz];
  float* dy = new float[nx * ny * nz];
  float* dz = new float[nx * ny * nz];
  float* d[3] = {dx, dy, dz};
  for (int x = 0; x < nx; x++) {
    for (int y = 0; y < ny; y++) {
      for (int z = 0; z < nz; z++) {
        int index = x + y * nx + z * nx * ny;
        float h = 2 * spacing[0];
        int i_m = getIndex(x - 1, y, z, nx, ny);
        int i_p = getIndex(x + 1, y, z, nx, ny);
        if (x == 0) {
          i_m = index;
          h = h / 2;
        }
        if (x == nx - 1) {
          i_p = index;
          h = h / 2;
        }
        dx[index] = (volume.data()[i_p] - volume.data()[i_m]) / h;

        h = 2 * spacing[1];
        i_m = getIndex(x, y - 1, z, nx, ny);
        i_p = getIndex(x, y + 1, z, nx, ny);
        if (y == 0) {
          i_m = index;
          h = h / 2;
        }
        if (y == ny - 1) {
          i_p = index;
          h = h / 2;
        }
        dy[index] = (volume.data()[i_p] - volume.data()[i_m]) / h;

        h = 2 * spacing[2];
        i_m = getIndex(x, y, z - 1, nx, ny);
        i_p = getIndex(x, y, z + 1, nx, ny);
        if (z == 0) {
          i_m = index;
          h = h / 2;
        }
        if (z == nz - 1) {
          i_p = index;
          h = h / 2;
        }
        dz[index] = (volume.data()[i_p] - volume.data()[i_m]) / h;
      }
    }
  }
  float coords[3];
  for (uint32_t i = 0; i < vertices.size<0>(); i++) {
    for (int coord = 0; coord < 3; coord++) {
      coords[coord] = (vertices(i, coord) - origin[coord]) / spacing[coord];
    }
    double len = 0;
    for (int coord = 0; coord < 3; coord++) {
      normals(i, coord) =
          sampleVolume(coords[0], coords[1], coords[2], nx, ny, nz, d[coord]);
      len += normals(i, coord) * normals(i, coord);
    }
    if (len != 0) {
      len = std::sqrt(len);
      for (int coord = 0; coord < 3; coord++) {
        normals(i, coord) = -normals(i, coord) / len;
      }
    }
  }

  delete[] dx;
  delete[] dy;
  delete[] dz;

  HANDLEDBUSPENDINGREPLY(prog.opGen().SetProgress(1.00, vx::emptyOptions()));
}

float AddNormalAttribute::sampleVolume(float x, float y, float z, int nx,
                                       int ny, int nz, const float* volume) {
  if (x < 0 || x > nx - 1) {
    return 0;
  }
  if (y < 0 || y > ny - 1) {
    return 0;
  }
  if (z < 0 || z > nz - 1) {
    return 0;
  }
  int x1 = std::floor(x);
  int x2 = std::ceil(x);
  int y1 = std::floor(y);
  int y2 = std::ceil(y);
  int z1 = std::floor(z);
  int z2 = std::ceil(z);
  float interpolateXY1 =
      volume[getIndex(x1, y1, z1, nx, ny)] * (1 - (x - x1)) * (1 - (y - y1)) +
      volume[getIndex(x2, y1, z1, nx, ny)] * (x - x1) * (1 - (y - y1)) +
      volume[getIndex(x1, y2, z1, nx, ny)] * (1 - (x - x1)) * (y - y1) +
      volume[getIndex(x2, y2, z1, nx, ny)] * (x - x1) * (y - y1);
  float interpolateXY2 =
      volume[getIndex(x1, y1, z2, nx, ny)] * (1 - (x - x1)) * (1 - (y - y1)) +
      volume[getIndex(x2, y1, z2, nx, ny)] * (x - x1) * (1 - (y - y1)) +
      volume[getIndex(x1, y2, z2, nx, ny)] * (1 - (x - x1)) * (y - y1) +
      volume[getIndex(x2, y2, z2, nx, ny)] * (x - x1) * (y - y1);
  return interpolateXY1 * (1 - (z - z1)) + interpolateXY2 * (z - z1);
}

int AddNormalAttribute::getIndex(int x, int y, int z, int nx, int ny) {
  return x + y * nx + z * nx * ny;
}
