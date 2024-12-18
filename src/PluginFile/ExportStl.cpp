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

#include "ExportStl.hpp"

#include <VoxieBackend/Data/SurfaceData.hpp>

#include <QtCore/QFile>

using namespace vx;
using namespace vx::file;

// Like QVector3D::normalized(), but with a smaller minimum size
static QVector3D normalized_small(const QVector3D& value) {
  double absSquared = double(value.x()) * double(value.x()) +
                      double(value.y()) * double(value.y()) +
                      double(value.z()) * double(value.z());
  if (absSquared >= 1e-50) {  // minimum length 1e-25
    double len = std::sqrt(absSquared);
    return QVector3D(float(double(value.x()) / len),
                     float(double(value.y()) / len),
                     float(double(value.z()) / len));
  } else {
    return QVector3D();
  }
}

static inline void write(QFile& file, const void* data, int length) {
  int pos = 0;
  while (pos < length) {
    int bytes =
        file.write(reinterpret_cast<const char*>(data) + pos, length - pos);
    if (bytes <= 0)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.IO.WriteError",
          "Error while writing to STL file: " + file.errorString());
    pos += bytes;
  }
}

[[gnu::unused]] static inline void write8(QFile& file, quint8 value) {
  write(file, &value, 1);
}
[[gnu::unused]] static inline void write16(QFile& file, quint16 value) {
  write(file, &value, 2);
}
static inline void write32(QFile& file, quint32 value) {
  write(file, &value, 4);
}
static inline void write(QFile& file, const QVector3D& vertex) {
  float x = vertex.x();
  float y = vertex.y();
  float z = vertex.z();
  write(file, &x, 4);
  write(file, &y, 4);
  write(file, &z, 4);
}

static void saveToSTL(SurfaceData* surface, const QString& filename) {
  auto srf = dynamic_cast<SurfaceDataTriangleIndexed*>(surface);
  if (!srf)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Cannot write data to STL file: Not a "
                        "SurfaceDataTriangleIndexed object");

  QFile stlFile(filename);
  stlFile.open(QIODevice::WriteOnly);

  char header[80];
  memset(header, 0, 80);
  write(stlFile, header, 80);
  if (srf->triangles().size() > std::numeric_limits<quint32>::max())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Cannot write data to STL file: More than 2^32-1 triangles");
  quint32 count = srf->triangles().size();
  write32(stlFile, count);

  for (size_t i = 0; i < srf->triangles().size(); i++) {
    const auto& triangle = srf->triangles()[i];
    QVector3D a = srf->vertices()[triangle[0]];
    QVector3D b = srf->vertices()[triangle[1]];
    QVector3D c = srf->vertices()[triangle[2]];

    QVector3D normal = normalized_small(QVector3D::crossProduct(b - c, c - a));

    write(stlFile, normal);

    write(stlFile, a);
    write(stlFile, b);
    write(stlFile, c);

    quint16 attrcount = 0;
    write16(stlFile, attrcount);
  }

  if (!stlFile.flush())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.IO.WriteError",
        "Error while flushing STL file: " + stlFile.errorString());

  stlFile.close();
}

ExportStl::ExportStl()
    : vx::io::Exporter("de.uni_stuttgart.Voxie.FileFormat.Stl.Export",
                       vx::io::FilenameFilter("Binary STL", {"*.stl"}),
                       {"de.uni_stuttgart.Voxie.Data.Surface"}) {}

QSharedPointer<vx::OperationResult> ExportStl::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  return runThreaded(
      "Export " + fileName,
      [data, fileName](const QSharedPointer<vx::io::Operation>& op) {
        auto surfaceData = qSharedPointerDynamicCast<vx::SurfaceData>(data);
        if (!surfaceData)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "ExportStl::exportData(): data is not a SurfaceData");

        (void)op;  // TODO: progress bar
        saveToSTL(surfaceData.data(), fileName);
      });
}
