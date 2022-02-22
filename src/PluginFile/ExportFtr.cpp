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

#include "ExportFtr.hpp"

#include <VoxieBackend/Data/SurfaceData.hpp>

#include <QtCore/QFile>

using namespace vx;
using namespace vx::file;

static void saveToFTR(SurfaceData* surface, const QString& filename) {
  auto srf = dynamic_cast<SurfaceDataTriangleIndexed*>(surface);
  if (!srf)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Cannot write data to FTR file: Not a "
                        "SurfaceDataTriangleIndexed object");

  QFile ftrFile(filename);
  ftrFile.open(QIODevice::WriteOnly);

  QTextStream out(&ftrFile);
  out.setRealNumberPrecision(9);

  out << "\n1\n(\n\npatch0\nempty\n)\n\n\n";

  out << srf->vertices().size() << "\n";
  out << "(\n";
  for (const auto& vertex : srf->vertices())
    out << "(" << vertex.x() << " " << vertex.y() << " " << vertex.z() << ")\n";
  out << ")\n\n\n";

  out << srf->triangles().size() << "\n";
  out << "(\n";
  for (const auto& triangle : srf->triangles())
    out << "((" << triangle[0] << " " << triangle[1] << " " << triangle[2]
        << ") 0)\n";
  out << ")\n\n";

  if (!ftrFile.flush())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.IO.WriteError",
        "Error while flushing FTR file: " + ftrFile.errorString());

  ftrFile.close();
}

ExportFtr::ExportFtr()
    : vx::io::Exporter("de.uni_stuttgart.Voxie.FileFormat.Ftr.Export",
                       vx::io::FilenameFilter("Freefoam FTR", {"*.ftr"}),
                       {"de.uni_stuttgart.Voxie.Data.Surface"}) {}

QSharedPointer<vx::OperationResult> ExportFtr::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  return runThreaded(
      "Export " + fileName,
      [data, fileName](const QSharedPointer<vx::io::Operation>& op) {
        auto surfaceData = qSharedPointerDynamicCast<vx::SurfaceData>(data);
        if (!surfaceData)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "ExportFtr::exportData(): data is not a SurfaceData");

        (void)op;  // TODO: progress bar
        saveToFTR(surfaceData.data(), fileName);
      });
}
