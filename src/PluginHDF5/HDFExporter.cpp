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

#include "HDFExporter.hpp"

#include <PluginHDF5/CT/DataFiles.hpp>

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

using namespace vx;

HDFExporter::HDFExporter()
    : vx::io::Exporter("de.uni_stuttgart.Voxie.FileFormat.Hdf5.Export",
                       vx::io::FilenameFilter("HDF5 file", {"*.hdf5", "*.h5"}),
                       {"de.uni_stuttgart.Voxie.Data.Volume"}) {}

HDFExporter::~HDFExporter() {}

// TODO: Also support Float16
// TODO: Also support Bool8
struct SupportedHDF5Types
    : vx::DataTypeList<vx::DataType::Float32, vx::DataType::Float64,
                       vx::DataType::Int8, vx::DataType::Int16,
                       vx::DataType::Int32, vx::DataType::Int64,
                       vx::DataType::UInt8, vx::DataType::UInt16,
                       vx::DataType::UInt32, vx::DataType::UInt64> {};

QSharedPointer<vx::OperationResult> HDFExporter::exportData(
    const QSharedPointer<vx::Data>& data, const QString& fileName) {
  return runThreaded(
      "Export " + fileName,
      [data, fileName](const QSharedPointer<vx::io::Operation>& op) {
        auto voxelData = qSharedPointerDynamicCast<VolumeDataVoxel>(data);
        // TODO: Non-voxel datasets?
        if (!voxelData)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "HDFExporter::exportData(): data is not a VolumeDataVoxel");

        (void)op;  // TODO: progress bar

        voxelData->performInGenericContextRestricted<SupportedHDF5Types>(
            [&](auto& dataInst) {
              typedef typename std::remove_reference<decltype(
                  dataInst)>::type::VoxelType Type;

              VolumeGen<Type, true> volume;
              volume.Type = "Volume";
              volume.GridOrigin = Math::Vector3<ldouble>(
                  voxelData->origin().x(), voxelData->origin().y(),
                  voxelData->origin().z());
              volume.GridSpacing = Math::DiagMatrix3<ldouble>(
                  voxelData->getSpacing().x(), voxelData->getSpacing().y(),
                  voxelData->getSpacing().z());
              volume.Volume.size[0] = voxelData->getDimensions().x;
              volume.Volume.size[1] = voxelData->getDimensions().y;
              volume.Volume.size[2] = voxelData->getDimensions().z;

              try {
                HDF5::matlabSerialize(fileName.toUtf8().data(), volume);
                size_t shape[3] = {voxelData->getDimensions().x,
                                   voxelData->getDimensions().y,
                                   voxelData->getDimensions().z};
                ptrdiff_t stridesBytes[3] = {
                    (ptrdiff_t)(sizeof(Type)),
                    (ptrdiff_t)(voxelData->getDimensions().x * sizeof(Type)),
                    (ptrdiff_t)(voxelData->getDimensions().x *
                                voxelData->getDimensions().y * sizeof(Type))};
                Math::ArrayView<const Type, 3> view(dataInst.getData(), shape,
                                                    stridesBytes);
                volume.Volume.write(view);

              } catch (Exception& e) {
                throw;
              } catch (std::exception& e) {
                throw Exception(
                    "de.uni_stuttgart.Voxie.HDFExporter.Error",
                    QString() + "Failure while saving file: " + e.what());
              }
            });
      });
}
