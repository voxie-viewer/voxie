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

#include <ExtFileHdf5/BoolHdf5.hpp>

#include <ExtFileHdf5/CT/DataFiles.hpp>
#include <ExtFileHdf5/CT/rawdatafiles.hpp>

#include <VoxieClient/Array.hpp>
#include <VoxieClient/Bool8.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DataTypeExt.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/JsonUtil.hpp>
#include <VoxieClient/QtUtil.hpp>

using namespace vx;

struct SupportedHDF5Types
    : vx::DataTypeExtList<
          vx::DataTypeMeta<vx::BaseType::Float, 16, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Float, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Float, 64, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Int, 8, vx::Endianness::None>,
          vx::DataTypeMeta<vx::BaseType::Int, 16, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Int, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Int, 64, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 8, vx::Endianness::None>,
          vx::DataTypeMeta<vx::BaseType::UInt, 16, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 64, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Bool, 8, vx::Endianness::None>> {};

void exportDataVolume(
    vx::DBusClient& dbusClient,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationExport>&
        op) {
  QString fileName = op.op().filename();
  auto inputDataPath = op.op().data();

  auto inputDataDynamicObject =
      makeSharedQObject<de::uni_stuttgart::Voxie::DynamicObject>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());
  auto inputDataSupportedInterfaces =
      inputDataDynamicObject->supportedInterfaces();
  bool found = false;
  for (const auto& interf : inputDataSupportedInterfaces) {
    if (interf == "de.uni_stuttgart.Voxie.VolumeDataVoxel") found = true;
  }
  // TODO: Non-voxel datasets?
  if (!found)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "HDFExporter::exportData(): data is not a VolumeDataVoxel");
  auto volumeData = makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
      dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());
  auto voxelData = makeSharedQObject<de::uni_stuttgart::Voxie::VolumeDataVoxel>(
      dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());

  vx::DataTypeExt dataType = parseDataTypeExt(volumeData->dataType());

  (void)op;  // TODO: progress bar

  vx::switchOverDataTypeExt<SupportedHDF5Types, void>(
      dataType, [&](auto traits) {
        using Traits = decltype(traits);
        using Type = typename Traits::Type;

        auto origin = volumeData->volumeOrigin();
        auto spacing = voxelData->gridSpacing();
        vx::Array3<const Type> data(HANDLEDBUSPENDINGREPLY(
            voxelData->GetDataReadonly(QMap<QString, QDBusVariant>())));

        VolumeGen<Type, true> volume;
        volume.Type = "Volume";
        volume.GridOrigin = Math::Vector3<ldouble>(
            std::get<0>(origin), std::get<1>(origin), std::get<2>(origin));
        volume.GridSpacing = Math::DiagMatrix3<ldouble>(
            std::get<0>(spacing), std::get<1>(spacing), std::get<2>(spacing));
        volume.Volume.size[0] = data.template size<0>();
        volume.Volume.size[1] = data.template size<1>();
        volume.Volume.size[2] = data.template size<2>();

        try {
          HDF5::matlabSerialize(fileName.toStdString(), volume);
          size_t shape[3] = {
              data.template size<0>(),
              data.template size<1>(),
              data.template size<2>(),
          };
          ptrdiff_t stridesBytes[3] = {
              data.template strideBytes<0>(),
              data.template strideBytes<1>(),
              data.template strideBytes<2>(),
          };
          Math::ArrayView<const Type, 3> view(data.data(), shape, stridesBytes);
          volume.Volume.write(view);

        } catch (Exception& e) {
          throw;
        } catch (std::exception& e) {
          throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                          QString() + "Failure while saving file: " + e.what());
        }
      });
}

void exportDataRaw(
    vx::DBusClient& dbusClient,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationExport>&
        op) {
  QString fileName = op.op().filename();
  auto inputDataPath = op.op().data();

  auto inputDataDynamicObject =
      makeSharedQObject<de::uni_stuttgart::Voxie::DynamicObject>(
          dbusClient.uniqueName(), inputDataPath.path(),
          dbusClient.connection());
  auto inputDataSupportedInterfaces =
      inputDataDynamicObject->supportedInterfaces();
  bool found = false;
  for (const auto& interf : inputDataSupportedInterfaces) {
    if (interf == "de.uni_stuttgart.Voxie.TomographyRawData2DAccessor")
      found = true;
  }
  if (!found)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "HDFExporter::exportData(): data is not a TomographyRawData2DAccessor");
  // auto volumeData = makeSharedQObject<de::uni_stuttgart::Voxie::VolumeData>(
  //    dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());
  auto accessorOp = makeSharedQObject<
      de::uni_stuttgart::Voxie::TomographyRawData2DAccessorOperations>(
      dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());

  // TODO: Allow selecting image kind
  auto imageKinds = HANDLEDBUSPENDINGREPLY(
      accessorOp->GetAvailableImageKinds(vx::emptyOptions()));
  if (imageKinds.size() < 1)
    throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                    "No available image kind");
  auto imageKind = imageKinds.at(imageKinds.size() - 1);
  auto imageKindJson = dbusToJson(imageKind);
  QString dimension = expectString(imageKindJson["Dimension"]);
  // qDebug() << dimension;

  // TODO: Allow selecting data type
  vx::DataTypeExt dataType(vx::BaseType::Float, 32, vx::Endianness::Native);

  auto geometry = dbusToJson(HANDLEDBUSPENDINGREPLY(
      accessorOp->GetGeometryData("ConeBeamCT", vx::emptyOptions())));
  QList<std::tuple<QString, quint64, double>> images;
  double pixelSizeX, pixelSizeY, dsa, dsd;
  size_t countX, countY;
  if (geometry.contains("Images")) {
    // qDebug() << "IMAGES";

    // TODO: Get from geometry data instead? (ProjectionSize etc.)
    auto metadata = dbusToJson(
        HANDLEDBUSPENDINGREPLY(accessorOp->GetMetadata(vx::emptyOptions())));
    auto info = expectObject(metadata["Info"]);
    auto pixelSize = expectArray(info["DetectorPixelSize"]);
    if (pixelSize.size() != 2)
      throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                      "DetectorPixelSize has not 2 elements");
    pixelSizeX = expectDouble(pixelSize[0]);
    pixelSizeY = expectDouble(pixelSize[1]);

    // TODO: Do proper inheritance for geometry, look in individual projection
    // for overwritten values
    auto globProjGeom = expectObject(geometry["ProjectionGeometry"]);
    vx::Vector<double, 3> detectorPosition{0, 0, 0};
    if (globProjGeom.contains("DetectorPosition"))
      detectorPosition = expectVector3(globProjGeom["DetectorPosition"]);
    vx::Vector<double, 3> sourcePosition =
        expectVector3(globProjGeom["SourcePosition"]);
    vx::Vector<double, 3> tablePosition =
        expectVector3(globProjGeom["TablePosition"]);
    dsa = sqrt(squaredNorm(tablePosition - sourcePosition));
    dsd = sqrt(squaredNorm(detectorPosition - sourcePosition));

    for (QJsonValue imageVal : expectArray(geometry["Images"])) {
      auto image = expectObject(imageVal);
      auto imgRef = expectObject(image["ImageReference"]);
      auto stream = expectString(imgRef["Stream"]);
      auto id = expectUnsignedInt(imgRef["ImageID"]);

      auto projGeom = expectObject(image["ProjectionGeometry"]);
      // TODO: Should this always require a value here?
      auto angle = expectDouble(projGeom["TableRotAngle"]);

      auto imageShape = HANDLEDBUSPENDINGREPLY(
          accessorOp->GetImageShape(stream, id, vx::emptyOptions()));
      if (images.size() == 0) {
        countX = std::get<0>(imageShape);
        countY = std::get<1>(imageShape);
      } else {
        if (countX != std::get<0>(imageShape) ||
            countY != std::get<1>(imageShape))
          throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                          "Non-regular image");
      }

      images.push_back(std::make_tuple(stream, id, angle));
    }
  } else {
    // TODO: This should be removed once all raw image providers provide proper
    // geometry information (in particular HDFImporter itself)

    // qDebug() << "NIMAGES";

    auto numberOfImages = HANDLEDBUSPENDINGREPLY(
        accessorOp->GetNumberOfImages("", vx::emptyOptions()));
    auto metadata = dbusToJson(
        HANDLEDBUSPENDINGREPLY(accessorOp->GetMetadata(vx::emptyOptions())));
    auto info = expectObject(metadata["Info"]);
    auto pixelSize = expectArray(info["DetectorPixelSize"]);
    if (pixelSize.size() != 2)
      throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                      "DetectorPixelSize has not 2 elements");
    pixelSizeX = expectDouble(pixelSize[0]);
    pixelSizeY = expectDouble(pixelSize[1]);
    auto geometryData = expectObject(info["Geometry"]);
    dsa = expectDouble(geometryData["DistanceSourceAxis"]);
    dsd = expectDouble(geometryData["DistanceSourceDetector"]);

    for (quint64 i = 0; i < numberOfImages; i++) {
      auto stream = "";
      auto id = i;

      auto perImgMetadata = dbusToJson(HANDLEDBUSPENDINGREPLY(
          accessorOp->GetPerImageMetadata(stream, id, vx::emptyOptions())));
      auto angle = expectDouble(perImgMetadata["Angle"]);

      images.push_back(std::make_tuple(stream, id, angle));

      auto imageShape = HANDLEDBUSPENDINGREPLY(
          accessorOp->GetImageShape(stream, id, vx::emptyOptions()));
      if (i == 0) {
        countX = std::get<0>(imageShape);
        countY = std::get<1>(imageShape);
      } else {
        if (countX != std::get<0>(imageShape) ||
            countY != std::get<1>(imageShape))
          throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                          "Non-regular image");
      }
    }
  }

  if (images.size() == 0)
    throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                    "No image found");

  vx::switchOverDataTypeExt<SupportedHDF5Types,
                            void>(dataType, [&](auto traits) {
    using Traits = decltype(traits);
    using Type = typename Traits::Type;

    try {
      RawGen<Type, true> raw;

      raw.Type = "SimpleConeBeamCTImageSequence";
      raw.DetectorPixelSizeX = pixelSizeX;
      raw.DetectorPixelSizeY = pixelSizeY;
      raw.DistanceSourceAxis = dsa;
      raw.DistanceSourceDetector = dsd;
      raw.Dimension = dimension.toStdString();
      raw.Angle = std::vector<double>(images.size());
      for (size_t i = 0; i < (size_t)images.size(); i++)
        (*raw.Angle)[i] = std::get<2>(images[i]);
      raw.Image.size[0] = countX;
      raw.Image.size[1] = countY;
      raw.Image.size[2] = images.size();

      // TODO: Set ObjectPosition and ObjectRotation?

      // Don't set MirroredYAxis

      HDF5::matlabSerialize(fileName.toStdString(), raw);

      // TODO: Multiple images at once?
      std::size_t maxSlices = 1;

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::TomographyRawData2DRegular>
          image(
              dbusClient,
              HANDLEDBUSPENDINGREPLY(
                  dbusClient.instance()->CreateTomographyRawData2DRegular(
                      dbusClient.clientPath(), std::make_tuple(countX, countY),
                      maxSlices, dataType.toTuple(), vx::emptyOptions())));
      using BufferType = Type;

      vx::Array3<const BufferType> array(
          HANDLEDBUSPENDINGREPLY(image->GetDataReadonly(vx::emptyOptions())));

      if (array.template size<0>() != countX)
        throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                        "array.template size<0>() != countX");
      if (array.template size<1>() != countY)
        throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                        "array.template size<1>() != countY");
      if (array.template strideBytes<0>() != (ptrdiff_t)sizeof(BufferType))
        throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                        "array.template strideBytes<0>() != "
                        "(ptrdiff_t)sizeof(BufferType)");
      if (array.template strideBytes<1>() !=
          (ptrdiff_t)sizeof(BufferType) * (ptrdiff_t)array.template size<0>())
        throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                        "array.template strideBytes<0>() != "
                        "(ptrdiff_t)sizeof(BufferType) * "
                        "(ptrdiff_t)array.template size<0>()");

      for (size_t i = 0; i < (size_t)images.size(); i++) {
        // TODO: Multiple images at once?
        std::size_t slices = 1;

        QList<std::tuple<QString, quint64>> imageList;
        imageList << std::make_tuple(std::get<0>(images[i]),
                                     std::get<1>(images[i]));
        HANDLEDBUSPENDINGREPLY(accessorOp->ReadImages(
            imageKind, imageList, std::make_tuple(0, 0),
            std::make_tuple(dbusClient.uniqueName(), image.path()), 0,
            std::make_tuple(0, 0), std::make_tuple(countX, countY),
            vx::emptyOptions()));

        HDF5::DataSpace memDataSpace =
            HDF5::DataSpace::createSimple(slices, countY, countX);
        HDF5::DataSpace fileDataSpace =
            HDF5::DataSpace::createSimple(images.size(), countY, countX);

        hsize_t start[3] = {i, 0, 0};
        hsize_t count[3] = {1, countY, countX};
        fileDataSpace.selectHyperslab(H5S_SELECT_SET, count, start);

        raw.Image.dataSet.write(array.data(), HDF5::getH5Type<BufferType>(),
                                memDataSpace, fileDataSpace);

        op.throwIfCancelled();
        HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
            1.0 * (i + 1) / images.size(), vx::emptyOptions()));
      }
    } catch (Exception& e) {
      throw;
    } catch (std::exception& e) {
      throw Exception("de.uni_stuttgart.Voxie.HDFExporter.Error",
                      QString() + "Failure while saving file: " + e.what());
    }
  });
}
