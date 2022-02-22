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

#include "HDFImporter.hpp"

#include <VoxieBackend/Data/TomographyRawData2DRegularInst.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#ifdef HDFIMPORTER_CPP_ADDITIONAL_INCLUDE
#include HDFIMPORTER_CPP_ADDITIONAL_INCLUDE
#endif

using namespace vx;
using namespace vx::io;

HDFImporter::HDFImporter()
    : Importer("de.uni_stuttgart.Voxie.HDFImporter",
               FilenameFilter("HDF5 Files", {"*.h5", "*.hdf5"}), {}) {}

HDFImporter::~HDFImporter() {}

QSharedPointer<vx::OperationResultImport> HDFImporter::import(
    const QString& fileName, bool doCreateNode,
    const QMap<QString, QVariant>& properties) {
  (void)properties;
  return runThreaded(
      "Import " + fileName, doCreateNode,
      [fileName](const QSharedPointer<vx::io::Operation>& op) {
        try {
          // check if file exists
          QFile qFile(fileName);
          if (!qFile.exists()) {
            throw Exception("de.uni_stuttgart.Voxie.HDFImporter.FileNotFound",
                            "File not found");
          }

          auto file =
              HDF5::File::open(fileName.toUtf8().data(), H5F_ACC_RDONLY);

          auto type = HDF5::matlabDeserialize<TypeCheckGen<float, true>>(file);
          // For no information, Assume this is a voxel volume file.
          if (!type->Type || *type->Type == "Volume") {
            return loadVoxelData(file, op);
          } else if (*type->Type == "SimpleConeBeamCTImageSequence") {
            return loadRawData(file, op);

#ifdef HDFIMPORTER_CPP_ADDITIONAL_IMPORT
            HDFIMPORTER_CPP_ADDITIONAL_IMPORT
#endif
          } else {
            throw Exception("de.uni_stuttgart.Voxie.HDFImporter.Error",
                            QString() + "Got unknown HDF5 file type: " +
                                type->Type->c_str());
          }
        } catch (Exception& e) {
          throw;
        } catch (std::exception& e) {
          throw Exception(
              "de.uni_stuttgart.Voxie.HDFImporter.Error",
              QString() + "Failure while loading file: " + e.what());
        }
      });
}

// Can throw arbitrary exceptions
QSharedPointer<Data> HDFImporter::loadVoxelData(
    const HDF5::File& file, const QSharedPointer<vx::io::Operation>& op) {
  DataType typeToUse;

  QSharedPointer<VolumeDataVoxel> voxelData;
  {
    auto volumeObj = file.rootGroup().open("Volume", HDF5::setEFilePrefix());
    HDF5::DataSet volume;
    if (volumeObj.getType() == H5I_GROUP)  // Octave data
      volume = (HDF5::DataSet)((HDF5::Group)volumeObj)
                   .open("value", HDF5::setEFilePrefix());
    else
      volume = (HDF5::DataSet)volumeObj;
    typeToUse = convertHDF5TypeToDataType(volume.getDataType());
    // create and fill the voxel data object
  }

  switch (typeToUse) {
    case DataType::Float64:
      voxelData = createGenericVolume<double>(file, typeToUse, op);
      break;
    case DataType::Float32:
      voxelData = createGenericVolume<float>(file, typeToUse, op);
      break;
    case DataType::Int8:
      voxelData = createGenericVolume<int8_t>(file, typeToUse, op);
      break;
    case DataType::Int16:
      voxelData = createGenericVolume<int16_t>(file, typeToUse, op);
      break;
    case DataType::Int32:
      voxelData = createGenericVolume<int32_t>(file, typeToUse, op);
      break;
    case DataType::Int64:
      voxelData = createGenericVolume<int64_t>(file, typeToUse, op);
      break;
    case DataType::UInt8:
      voxelData = createGenericVolume<uint8_t>(file, typeToUse, op);
      break;
    case DataType::UInt16:
      voxelData = createGenericVolume<uint16_t>(file, typeToUse, op);
      break;
    case DataType::UInt32:
      voxelData = createGenericVolume<uint32_t>(file, typeToUse, op);
      break;
    case DataType::UInt64:
      voxelData = createGenericVolume<uint64_t>(file, typeToUse, op);
      break;

    default:
      qWarning() << "Float16 and Boolean not yet implemented";
      voxelData = createGenericVolume<float>(file, typeToUse, op);
  }

  return voxelData;
}

TomographyRawData2DAccessorHDF5::TomographyRawData2DAccessorHDF5(
    const HDF5::File& file)
    : file(file) {
  raw = HDF5::matlabDeserialize<RawGen<float, true>>(file);

  numberOfImages_ = raw->Image.size[2];
  imageSize_ = VectorSizeT2(raw->Image.size[0], raw->Image.size[1]);
  // qDebug() << "numberOfImages" << numberOfImages_ << "imageSize" <<
  // imageSize_.x << imageSize_.y;
  QJsonObject geometry;
  if (raw->DistanceSourceAxis)
    geometry["DistanceSourceAxis"] = *raw->DistanceSourceAxis;
  if (raw->DistanceSourceDetector)
    geometry["DistanceSourceDetector"] = *raw->DistanceSourceDetector;
  QJsonObject info{
      {"Geometry", geometry},
  };
  if (raw->DetectorPixelSizeX && raw->DetectorPixelSizeY) {
    info["DetectorPixelSize"] = QJsonArray{
        *raw->DetectorPixelSizeX,
        *raw->DetectorPixelSizeY,
    };
  }
  metadata_ = QJsonObject{
      {"Info", info},
  };
  if (raw->Angle) {
    angles = *raw->Angle;
    if (angles.size() != numberOfImages_)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Number of angles does not match number of images");
  } else {
    angles.resize(numberOfImages_);
    for (size_t i = 0; i < numberOfImages_; i++)
      angles[i] = 2.0 * M_PI / numberOfImages_ * i;
  }
  for (size_t i = 0; i < numberOfImages_; i++) {
    QJsonObject map{
        {"Angle", angles[i]},
    };
    perImageMetadata_[i] = map;
  }
  QJsonObject imageKind;
  if (!raw->Dimension) {
    imageKind["Description"] = "Unknown";
    imageKind["Dimension"] = "Unknown";
    imageKind["Corrections"] = QJsonObject();
  } else if (*raw->Dimension == "Intensity") {
    imageKind["Description"] =
        "Non-normalized intensity image with bad pixel and shading correction";
    imageKind["Dimension"] = "Intensity";
    QJsonObject corrections;
    corrections["NormalizeLevel"] = false;
    corrections["BadPixel"] = true;
    corrections["Shading"] = true;
    // corrections["FluxNormalizaion"] = false; // unknown
    imageKind["Corrections"] = corrections;
  } else if (*raw->Dimension == "Attenuation") {
    imageKind["Description"] =
        "Attenuation image with bad pixel and shading correction";
    imageKind["Dimension"] = "Attenuation";
    QJsonObject corrections;
    corrections["NormalizeLevel"] = true;
    corrections["BadPixel"] = true;
    corrections["Shading"] = true;
    // corrections["FluxNormalizaion"] = false; // unknown
    imageKind["Corrections"] = corrections;
  } else {
    imageKind["Description"] =
        QString() + "Unknown (\"" + raw->Dimension->c_str() + "\")";
    imageKind["Dimension"] = "Unknown";
    imageKind["Corrections"] = QJsonObject();
  }
  availableImageKinds_ << imageKind;

  availableStreams_ << "";
  availableGeometryTypes_ << "ConeBeamCT";  // TODO?
}

TomographyRawData2DAccessorHDF5::~TomographyRawData2DAccessorHDF5() {}

// Can throw arbitrary exceptions
QSharedPointer<Data> HDFImporter::loadRawData(
    const HDF5::File& file, const QSharedPointer<vx::io::Operation>& op) {
  // TODO: use progress bar?
  Q_UNUSED(op);

  return TomographyRawData2DAccessorHDF5::create(file);
}

// TODO: Move code into TomographyRawData2DAccessor?
vx::VectorSizeT2 TomographyRawData2DAccessorHDF5::imageSize(
    const QString& stream, uint64_t id) {
  if (stream != "")
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");
  if (id >= numberOfImages("" /*TODO*/)) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Image ID " + QString::number(id) + " is out of range");
  }
  return imageSize_;
}
QJsonObject TomographyRawData2DAccessorHDF5::getPerImageMetadata(
    const QString& stream, uint64_t id) {
  if (stream != "")
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");
  if (id >= (uint64_t)perImageMetadata_.size()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Image ID " + QString::number(id) + " is out of range");
  }
  return perImageMetadata_[id];
}
QJsonObject TomographyRawData2DAccessorHDF5::getGeometryData(
    const QString& geometryType) {
  // TODO: Return something here?
  if (geometryType != "ConeBeamCT")
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Invalid geometry type");
  return QJsonObject();
}

uint64_t TomographyRawData2DAccessorHDF5::numberOfImages(
    const QString& stream) {
  if (stream != "")
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");
  return numberOfImages_;
}

QString TomographyRawData2DAccessorHDF5::readImages(
    const QJsonObject& imageKind,
    const QList<std::tuple<QString, quint64>>& images,
    const std::tuple<quint64, quint64>& inputRegionStart,
    const QSharedPointer<TomographyRawData2DRegular>& output,
    qulonglong firstOutputImageId,
    const std::tuple<quint64, quint64>& outputRegionStart,
    const std::tuple<quint64, quint64>& regionSize, bool allowIncompleteData) {
  (void)allowIncompleteData;

  // TODO: load only the needed part of the image?
  // TODO: Support non-float images?
  Q_UNUSED(imageKind);  // TODO: use (and check) imageKind
  try {
    auto imageShape = output->imageShape();

    // ASSERT(std::get<0>(inputRegionStart) >= 0); // Always positive, is
    // unsigned ASSERT(std::get<1>(inputRegionStart) >= 0); // Always positive,
    // is unsigned
    ASSERT(std::get<0>(inputRegionStart) <= imageSize_.x);
    ASSERT(std::get<1>(inputRegionStart) <= imageSize_.y);
    // ASSERT(std::get<0>(outputRegionStart) >= 0); // Always positive, is
    // unsigned ASSERT(std::get<1>(outputRegionStart) >= 0); // Always positive,
    // is unsigned
    ASSERT(std::get<0>(outputRegionStart) <= std::get<0>(imageShape));
    ASSERT(std::get<1>(outputRegionStart) <= std::get<1>(imageShape));
    // ASSERT(std::get<0>(regionSize) >= 0); // Always positive, is unsigned
    // ASSERT(std::get<1>(regionSize) >= 0); // Always positive, is unsigned
    ASSERT((size_t)std::get<0>(inputRegionStart) +
               (size_t)std::get<0>(regionSize) <=
           imageSize_.x);
    ASSERT((size_t)std::get<1>(inputRegionStart) +
               (size_t)std::get<1>(regionSize) <=
           imageSize_.y);
    ASSERT((size_t)std::get<0>(outputRegionStart) +
               (size_t)std::get<0>(regionSize) <=
           std::get<0>(imageShape));
    ASSERT((size_t)std::get<1>(outputRegionStart) +
               (size_t)std::get<1>(regionSize) <=
           std::get<1>(imageShape));

    // TODO: Check firstOutputImageId / number of images / ...

    auto update = output->createUpdate();

    size_t imagePos = firstOutputImageId;
    for (const auto& entry : images) {
      const auto& stream = std::get<0>(entry);
      const auto& id = std::get<1>(entry);

      // TODO: Support multiple streams?
      if (stream != "")
        throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");

      // TODO: Load multiple images in one step?
      // TODO: clean this up: There should be no need to use a transposed image
      Math::Array<float, 3> data(imageSize_.x, imageSize_.y, 1);
      loadAndTransformSingleTo<
        float /*should be float even if half or integer type is used for volume data*/>(
        *raw, data.view(), id);

      output->performInGenericContext([&](const auto& raw) {
        using ValueType =
            typename std::remove_reference<decltype(*raw)>::type::ValueType;

        const auto& array = raw->array();

        for (size_t j = 0; j < std::get<0>(regionSize); j++) {
          for (size_t k = 0; k < std::get<1>(regionSize); k++) {
            array(std::get<0>(outputRegionStart) + j,
                  std::get<1>(outputRegionStart) + k, imagePos) =
                static_cast<ValueType>(data(std::get<0>(inputRegionStart) + j,
                                            std::get<1>(outputRegionStart) + k,
                                            0));
          }
        }
      });

      imagePos++;
    }

    auto version = update->finish(QJsonObject{});
    return version->versionString();
  } catch (Exception& e) {
    throw;
  } catch (std::exception& e) {
    throw Exception("de.uni_stuttgart.Voxie.HDFImporter.Error",
                    QString() + "Failure while loading image: " + e.what());
  }
}

QList<QSharedPointer<SharedMemory>>
TomographyRawData2DAccessorHDF5::getSharedMemorySections() {
  return {};
}

DataType HDFImporter::convertHDF5TypeToDataType(HDF5::DataType type) {
  if (type.getClass() == H5T_FLOAT) {
    switch (type.getSize()) {
      case 8:
        return DataType::Float64;
      case 4:
        return DataType::Float32;
      case 2:
        return DataType::Float16;
    }
  } else if (type.getClass() == H5T_INTEGER) {
    if (type.getSign() == H5T_SGN_NONE) {
      switch (type.getSize()) {
        case 8:
          return DataType::UInt64;
        case 4:
          return DataType::UInt32;
        case 2:
          return DataType::UInt16;
        case 1:
          return DataType::UInt8;
      }
    } else {
      switch (type.getSize()) {
        case 8:
          return DataType::Int64;
        case 4:
          return DataType::Int32;
        case 2:
          return DataType::Int16;
        case 1:
          return DataType::Int8;
      }
    }
  }
  qWarning() << "Unknow HDF5::DataType. Falling back to DataType::Float32.";
  return DataType::Float32;
}
