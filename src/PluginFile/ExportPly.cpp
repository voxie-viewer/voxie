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

#include "ExportPly.hpp"

// Note: This exporter is unused (ExtFilePly is used instead)

#include <VoxieBackend/Data/SurfaceData.hpp>

#include <QtCore/QFile>

using namespace vx;
using namespace vx::file;

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

static inline void write8(QFile& file, quint8 value) { write(file, &value, 1); }
static inline void write16(QFile& file, quint16 value) {
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

// https://en.wikipedia.org/w/index.php?title=PLY_(file_format)&oldid=717727643
static void saveToPLY(SurfaceData* surface, const QString& filename) {
  auto srf = dynamic_cast<SurfaceDataTriangleIndexed*>(surface);
  if (!srf)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Cannot write data to PLY file: Not a "
                        "SurfaceDataTriangleIndexed object");

  QFile plyFile(filename);
  plyFile.open(QIODevice::WriteOnly);

  {
    QTextStream out(&plyFile);
    out.setRealNumberPrecision(9);

    out << "ply\n";
    if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
      out << "format binary_big_endian 1.0\n";
    } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
      out << "format binary_little_endian 1.0\n";
    } else {
      throw vx::Exception("de.uni_stuttgart.Voxie.IO.WriteError",
                          "Unknown endianness");
    }

    out << "comment Generated by Voxie\n";

    out << "element vertex " << srf->vertices().size() << "\n";
    out << "property float32 x\n";
    out << "property float32 y\n";
    out << "property float32 z\n";

    out << "element face " << srf->triangles().size() << "\n";
    out << "property list uint8 uint32 vertex_indices\n";

    out << "end_header\n";
  }

  for (const auto& vertex : srf->vertices()) write(plyFile, vertex);

  for (const auto& triangle : srf->triangles()) {
    write8(plyFile, 3);  // Number of vertices
    write32(plyFile, triangle[0]);
    write32(plyFile, triangle[1]);
    write32(plyFile, triangle[2]);
  }

  if (!plyFile.flush())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.IO.WriteError",
        "Error while flushing PLY file: " + plyFile.errorString());

  plyFile.close();
}

ExportPly::ExportPly()
    : vx::io::Exporter("de.uni_stuttgart.Voxie.FileFormat.Ply.Export",
                       vx::io::FilenameFilter("Stanford PLY (old)", {"*.ply"}),
                       {"de.uni_stuttgart.Voxie.Data.Surface"}) {}

QSharedPointer<vx::OperationResult> ExportPly::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  return runThreaded(
      "Export " + fileName,
      [data, fileName](const QSharedPointer<vx::io::Operation>& op) {
        auto surfaceData = qSharedPointerDynamicCast<vx::SurfaceData>(data);
        if (!surfaceData)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "ExportPly::exportData(): data is not a SurfaceData");

        (void)op;  // TODO: progress bar
        saveToPLY(surfaceData.data(), fileName);
      });
}