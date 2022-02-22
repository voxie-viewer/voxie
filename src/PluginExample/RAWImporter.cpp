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

#include "RAWImporter.hpp"

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/IO/Operation.hpp>

#include <cmath>
#include <cstdlib>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

using namespace vx;
using namespace vx::io;
using namespace vx;

RAWImporter::RAWImporter()
    : Importer("de.uni_stuttgart.Voxie.Example.RAWImporter",
               FilenameFilter("RAW Files", {"*.raw"}), {}) {}

static inline size_t divrem(size_t value, size_t divisor, size_t& rL) {
  std::lldiv_t result = std::lldiv(value, divisor);
  rL = result.rem;
  return result.quot;
}

static bool isPowerOfThree(size_t nL) {
  // General algorithm only
  size_t rL, r;
  nL = divrem(nL, 3486784401, rL);
  if (nL != 0 && rL != 0) return false;
  nL = divrem(nL + rL, 59049, rL);
  if (nL != 0 && rL != 0) return false;
  size_t n = (size_t)nL + (size_t)rL;
  n = divrem(n, 243, r);
  if (n != 0 && r != 0) return false;
  n = divrem(n + r, 27, r);
  if (n != 0 && r != 0) return false;
  n += r;
  return n == 1 || n == 3 || n == 9;
}

QSharedPointer<vx::OperationResultImport> RAWImporter::import(
    const QString& fileName, bool doCreateNode,
    const QMap<QString, QVariant>& properties) {
  (void)properties;
  return runThreaded(
      "Import " + fileName, doCreateNode,
      [fileName](const QSharedPointer<vx::io::Operation>& op) {
        QFile file(fileName);

        size_t fileSize = file.size();

        if (file.exists() == false) {
          throw Exception("de.uni_stuttgart.Voxie.RAWImporter.FileNotFound",
                          "File not found");
        }

        if (fileSize == 0) {
          throw Exception("de.uni_stuttgart.Voxie.RAWImporter.Error",
                          "File must not be empty");
        }

        if (isPowerOfThree(fileSize >> 2) == false) {
          throw Exception("de.uni_stuttgart.Voxie.RAWImporter.Error",
                          "File must be a cubic data set");
        }

        size_t size = std::pow(fileSize >> 2, 1.0f / 3.0f);
        if ((size * size * size) != (fileSize >> 2)) {
          throw Exception("de.uni_stuttgart.Voxie.RAWImporter.Error",
                          "Data set dimensions are too large for double");
        }

        auto data = VolumeDataVoxel::createVolume(size, size, size);
        if (file.open(QFile::ReadOnly) == false) {
          throw Exception("de.uni_stuttgart.Voxie.RAWImporter.Error",
                          "Failed to open file");
        }

        op->throwIfCancelled();

        data->performInGenericContext([&](auto& data2) {
          if (static_cast<size_t>(file.read(
                  reinterpret_cast<char*>(data2.getData()), fileSize)) !=
              fileSize) {
            throw Exception("de.uni_stuttgart.Voxie.RAWImporter.Error",
                            "Failed to read whole file");
          }
        });

        op->updateProgress(1.0);

        file.close();

        return data;
      });
}
