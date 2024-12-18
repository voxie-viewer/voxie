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

#include <ExtFileHdf5/BoolHdf5.hpp>

#include <VoxieClient/Array.hpp>
#include <VoxieClient/Bool8.hpp>
#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/JsonDBus.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <HDF5/EnumType.hpp>

#include <half.hpp>

#ifdef HDFIMPORTER_CPP_ADDITIONAL_INCLUDE
#include HDFIMPORTER_CPP_ADDITIONAL_INCLUDE
#endif

using namespace vx;

// TODO: Move somewhere else?
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

// TODO: Move somewhere else?
template <typename T, size_t N, size_t pos = 0>
struct ToTupleImpl {
  static void copy(const vx::Vector<T, N> src, vx::TupleVector<T, N>& dst) {
    std::get<pos>(dst) = src.template access<pos>();
    ToTupleImpl<T, N, pos + 1>::copy(src, dst);
  }
};
template <typename T, size_t N>
struct ToTupleImpl<T, N, N> {
  static void copy(const vx::Vector<T, N> src, vx::TupleVector<T, N>& dst) {
    (void)src;
    (void)dst;
  }
};
template <typename T, size_t N>
static inline vx::TupleVector<T, N> toTuple(const vx::Vector<T, N>& vec) {
  vx::TupleVector<T, N> res;
  ToTupleImpl<T, N>::copy(vec, res);
  return res;
}

// TODO: Move somewhere else?
template <typename T>
vx::Vector<T, 3> toVector(const Math::Vector3<T>& vec) {
  return {vec.x(), vec.y(), vec.z()};
}
template <typename T>
vx::Vector<T, 3> toVector(const Math::DiagMatrix3<T>& m) {
  return {m.m11(), m.m22(), m.m33()};
}

static std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
                  vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
createGenericVolume(
    vx::DBusClient& dbusClient, HDF5::File file, DataTypeExt typeToUse,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
        op) {
  // Note in order to workaround
  // https://developercommunity.microsoft.com/t/Captured-parameter-not-visible-in-nested/10634637
  // all captured variable have to be listed explicitly (in particular, op has
  // to be listed)
  return vx::switchOverDataTypeExt<
      // AllSupportedTypesVolume,
      SupportedHDF5Types,
      std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
                 vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>>(
      typeToUse, [&dbusClient, &file, &typeToUse, &op](auto traits) {
        using Traits = decltype(traits);
        using Type = typename Traits::Type;

        std::shared_ptr<VolumeGen<Type, true>> volume =
            HDF5::matlabDeserialize<VolumeGen<Type, true>>(file);
        Math::Vector3<size_t> size = getSize(*volume);

        // read meta data
        vx::Vector<double, 3> origin(0, 0, 0);
        if (volume->GridOrigin)
          origin = vectorCastNarrow<double>(toVector(*volume->GridOrigin));
        vx::Vector<double, 3> spacing(1, 1, 1);
        if (volume->GridSpacing)
          spacing = vectorCastNarrow<double>(toVector(*volume->GridSpacing));

        // create and fill the voxel data object
        vx::RefObjWrapper<de::uni_stuttgart::Voxie::VolumeDataVoxel> voxelData(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(dbusClient.instance()->CreateVolumeDataVoxel(
                dbusClient.clientPath(),
                std::make_tuple(size.x(), size.y(), size.z()),
                typeToUse.toTuple(), toTuple(origin), toTuple(spacing),
                vx::emptyOptions())));
        auto data = voxelData.castUnchecked<de::uni_stuttgart::Voxie::Data>();
        // VolumeDataVoxel::createVolume(size.x(), size.y(), size.z(),
        // typeToUse);

        vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
            dbusClient, HANDLEDBUSPENDINGREPLY(data->CreateUpdate(
                            dbusClient.clientPath(), vx::emptyOptions())));

        vx::Array3<Type> array(HANDLEDBUSPENDINGREPLY(
            voxelData->GetDataWritable(update.path(), vx::emptyOptions())));

        size_t shape[3] = {
            array.template size<0>(),
            array.template size<1>(),
            array.template size<2>(),
        };
        ptrdiff_t stridesBytes[3] = {
            array.template strideBytes<0>(),
            array.template strideBytes<1>(),
            array.template strideBytes<2>(),
        };
        Math::ArrayView<Type, 3> view(array.data(), shape, stridesBytes);
        // Using op2 iw a workaround for
        // https://developercommunity.microsoft.com/t/Captured-parameter-not-visible-in-nested/10634637
        auto& op2 = op;
        loadAndTransformTo<Type>(
            *volume, view, [&op2](size_t pos, size_t count) {
              op2.throwIfCancelled();
              HANDLEDBUSPENDINGREPLY(op2.opGen().SetProgress(
                  1.0 * pos / count, vx::emptyOptions()));
            });

        vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> newVersion(
            dbusClient, HANDLEDBUSPENDINGREPLY(update->Finish(
                            dbusClient.clientPath(), vx::emptyOptions())));

        return std::make_tuple(data, newVersion);
      });
}

// Can throw arbitrary exceptions
std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
           vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
loadVoxelData(
    vx::DBusClient& dbusClient, const HDF5::File& file,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
        op) {
  DataTypeExt typeToUse(vx::BaseType::Float, 32, vx::Endianness::Native);

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

  return createGenericVolume(dbusClient, file, typeToUse, op);
}

std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
           vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
import(vx::DBusClient& dbusClient,
       vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
           op) {
  QString fileName = op.op().filename();

  try {
    // check if file exists
    QFile qFile(fileName);
    if (!qFile.exists()) {
      throw Exception("de.uni_stuttgart.Voxie.HDFImporter.FileNotFound",
                      "File not found");
    }

    auto file = HDF5::File::open(fileName.toUtf8().data(), H5F_ACC_RDONLY);

    auto type = HDF5::matlabDeserialize<TypeCheckGen<float, true>>(file);
    // For no information, Assume this is a voxel volume file.
    if (!type->Type || *type->Type == "Volume") {
      return loadVoxelData(dbusClient, file, op);
    } else if (*type->Type == "SimpleConeBeamCTImageSequence") {
      return loadRawData(dbusClient, file, op);

#ifdef HDFIMPORTER_CPP_ADDITIONAL_IMPORT
      HDFIMPORTER_CPP_ADDITIONAL_IMPORT
#endif
    } else {
      throw Exception(
          "de.uni_stuttgart.Voxie.HDFImporter.Error",
          QString() + "Got unknown HDF5 file type: " + type->Type->c_str());
    }
  } catch (Exception& e) {
    throw;
  } catch (std::exception& e) {
    throw Exception("de.uni_stuttgart.Voxie.HDFImporter.Error",
                    QString() + "Failure while loading file: " + e.what());
  }
}

class TomographyRawData2DAccessorHDF5 : public RefCountedObject {
  VX_REFCOUNTEDOBJECT

  DBusClient& dbusClientRef;  // TODO: Don't keep a reference here
  QDBusConnection my_connection;
  HDF5::File file;
  std::shared_ptr<RawGen<float, true>> raw;

  uint64_t numberOfImages_;
  QJsonObject metadata_;
  QList<QJsonObject> availableImageKinds_;
  QList<QString> availableStreams_;
  QList<QString> availableGeometryTypes_;
  vx::Vector<quint64, 2> imageSize_;
  std::vector<double> angles;
  QMap<uint64_t, QJsonObject> perImageMetadata_;

 public:
  TomographyRawData2DAccessorHDF5(vx::DBusClient& dbusClient,
                                  const HDF5::File& file);
  virtual ~TomographyRawData2DAccessorHDF5();

  uint64_t numberOfImages(const QString& stream);
  QJsonObject metadata() const { return metadata_; }
  QList<QJsonObject> availableImageKinds() const {
    return availableImageKinds_;
  }
  QList<QString> availableStreams() { return availableStreams_; }
  bool hasStream(const QString& stream) { return stream == ""; }
  QList<QString> availableGeometryTypes() { return availableGeometryTypes_; }
  vx::Vector<quint64, 2> imageShape(const QString& stream, uint64_t id);
  QJsonObject getPerImageMetadata(const QString& stream, uint64_t id);
  QJsonObject getGeometryData(const QString& geometryType);

  QString readImages(const QJsonObject& imageKind,
                     const QList<std::tuple<QString, quint64>>& images,
                     const std::tuple<quint64, quint64>& inputRegionStart,
                     const std::tuple<QString, QDBusObjectPath>& output,
                     quint64 firstOutputImageId,
                     const std::tuple<quint64, quint64>& outputRegionStart,
                     const std::tuple<quint64, quint64>& regionSize,
                     bool allowIncompleteData);
};

// TODO: Move somewhere else?
class TomographyRawData2DAccessorOperationsAdaptorImpl
    : public TomographyRawData2DAccessorOperationsAdaptor {
  TomographyRawData2DAccessorHDF5* object;

 public:
  TomographyRawData2DAccessorOperationsAdaptorImpl(
      TomographyRawData2DAccessorHDF5* object)
      : TomographyRawData2DAccessorOperationsAdaptor(object), object(object) {}

  QStringList GetAvailableGeometryTypes(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return object->availableGeometryTypes();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QList<QMap<QString, QDBusVariant>> GetAvailableImageKinds(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      QList<QMap<QString, QDBusVariant>> kinds;
      for (const auto& kind : object->availableImageKinds()) {
        kinds << jsonToDBus(kind);
      }
      return kinds;
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QStringList GetAvailableStreams(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return object->availableStreams();
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QMap<QString, QDBusVariant> GetGeometryData(
      const QString& geometryType,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return jsonToDBus(object->getGeometryData(geometryType));
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  std::tuple<quint64, quint64> GetImageShape(
      const QString& stream, quint64 id,
      const QMap<QString, QDBusVariant>& options) override {
    Q_UNUSED(id);
    try {
      ExportedObject::checkOptions(options);
      return toTuple(object->imageShape(stream, id));
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QMap<QString, QDBusVariant> GetMetadata(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return jsonToDBus(object->metadata());
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  quint64 GetNumberOfImages(
      const QString& stream,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return object->numberOfImages(stream);
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QMap<QString, QDBusVariant> GetPerImageMetadata(
      const QString& stream, quint64 id,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);
      return jsonToDBus(object->getPerImageMetadata(stream, id));
    } catch (Exception& e) {
      return e.handle(object);
    }
  }

  QString ReadImages(const QMap<QString, QDBusVariant>& imageKind,
                     const QList<std::tuple<QString, quint64>>& images,
                     const std::tuple<quint64, quint64>& inputRegionStart,
                     const std::tuple<QString, QDBusObjectPath>& output,
                     quint64 firstOutputImageId,
                     const std::tuple<quint64, quint64>& outputRegionStart,
                     const std::tuple<quint64, quint64>& regionSize,
                     const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowIncompleteData");

      auto allowIncompleteData = ExportedObject::getOptionValueOrDefault<bool>(
          options, "AllowIncompleteData", false);

      return object->readImages(dbusToJson(imageKind), images, inputRegionStart,
                                output, firstOutputImageId, outputRegionStart,
                                regionSize, allowIncompleteData);
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};

TomographyRawData2DAccessorHDF5::TomographyRawData2DAccessorHDF5(
    vx::DBusClient& dbusClient, const HDF5::File& file)
    : RefCountedObject("TomographyRawData2DAccessor"),
      dbusClientRef(dbusClient),
      my_connection(dbusClient.connection()),
      file(file) {
  new TomographyRawData2DAccessorOperationsAdaptorImpl(this);

  raw = HDF5::matlabDeserialize<RawGen<float, true>>(file);

  numberOfImages_ = raw->Image.size[2];
  imageSize_ = {raw->Image.size[0], raw->Image.size[1]};
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
std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
           vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
loadRawData(
    vx::DBusClient& dbusClient, const HDF5::File& file,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
        op) {
  // TODO: use progress bar?
  Q_UNUSED(op);

  auto provider = TomographyRawData2DAccessorHDF5::create(dbusClient, file);

  vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data> outputDataWrapper(
      dbusClient,
      HANDLEDBUSPENDINGREPLY(dbusClient->CreateTomographyRawData2DAccessor(
          dbusClient.clientPath(),
          std::make_tuple(dbusClient.connection().baseService(),
                          provider->getPath()),
          vx::emptyOptions())));

  vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> versionWrapper(
      dbusClient, HANDLEDBUSPENDINGREPLY(outputDataWrapper->GetCurrentVersion(
                      dbusClient.clientPath(), vx::emptyOptions())));

  return std::make_tuple(outputDataWrapper, versionWrapper);
}

// TODO: Move code into TomographyRawData2DAccessor?
vx::Vector<quint64, 2> TomographyRawData2DAccessorHDF5::imageShape(
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
    const std::tuple<QString, QDBusObjectPath>& outputRef,
    quint64 firstOutputImageId,
    const std::tuple<quint64, quint64>& outputRegionStart,
    const std::tuple<quint64, quint64>& regionSize, bool allowIncompleteData) {
  (void)allowIncompleteData;

  de::uni_stuttgart::Voxie::TomographyRawData2DRegular output(
      std::get<0>(outputRef), std::get<1>(outputRef).path(), my_connection);
  de::uni_stuttgart::Voxie::Data outputData(
      std::get<0>(outputRef), std::get<1>(outputRef).path(), my_connection);

  // TODO: load only the needed part of the image?
  // TODO: Support non-float images?
  Q_UNUSED(imageKind);  // TODO: use (and check) imageKind
  try {
    auto imageShape = output.imageShape();

    // ASSERT(std::get<0>(inputRegionStart) >= 0); // Always positive, is
    // unsigned ASSERT(std::get<1>(inputRegionStart) >= 0); // Always positive,
    // is unsigned
    ASSERT(std::get<0>(inputRegionStart) <= imageSize_.access<0>());
    ASSERT(std::get<1>(inputRegionStart) <= imageSize_.access<1>());
    // ASSERT(std::get<0>(outputRegionStart) >= 0); // Always positive, is
    // unsigned ASSERT(std::get<1>(outputRegionStart) >= 0); // Always positive,
    // is unsigned
    ASSERT(std::get<0>(outputRegionStart) <= std::get<0>(imageShape));
    ASSERT(std::get<1>(outputRegionStart) <= std::get<1>(imageShape));
    // ASSERT(std::get<0>(regionSize) >= 0); // Always positive, is unsigned
    // ASSERT(std::get<1>(regionSize) >= 0); // Always positive, is unsigned
    ASSERT((size_t)std::get<0>(inputRegionStart) +
               (size_t)std::get<0>(regionSize) <=
           imageSize_.access<0>());
    ASSERT((size_t)std::get<1>(inputRegionStart) +
               (size_t)std::get<1>(regionSize) <=
           imageSize_.access<1>());
    ASSERT((size_t)std::get<0>(outputRegionStart) +
               (size_t)std::get<0>(regionSize) <=
           std::get<0>(imageShape));
    ASSERT((size_t)std::get<1>(outputRegionStart) +
               (size_t)std::get<1>(regionSize) <=
           std::get<1>(imageShape));

    // TODO: Check firstOutputImageId / number of images / ...

    auto outputType = parseDataTypeExt(output.dataType());

    vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
        dbusClientRef, HANDLEDBUSPENDINGREPLY(outputData.CreateUpdate(
                           dbusClientRef.clientPath(), vx::emptyOptions())));

    size_t imagePos = firstOutputImageId;
    for (const auto& entry : images) {
      const auto& stream = std::get<0>(entry);
      const auto& id = std::get<1>(entry);

      // TODO: Support multiple streams?
      if (stream != "")
        throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");

      // TODO: Load multiple images in one step?
      // TODO: clean this up: There should be no need to use a transposed image
      Math::Array<float, 3> data(imageSize_.access<0>(), imageSize_.access<1>(),
                                 1);
      loadAndTransformSingleTo<
        float /*should be float even if half or integer type is used for volume data*/>(
        *raw, data.view(), id);

      vx::switchOverDataTypeExt<AllSupportedTypesRawData, void>(
          outputType, [&](auto traits) {
            using Traits = decltype(traits);
            using ValueType = typename Traits::Type;

            vx::Array3<ValueType> array(HANDLEDBUSPENDINGREPLY(
                output.GetDataWritable(update.path(), vx::emptyOptions())));

            for (size_t j = 0; j < std::get<0>(regionSize); j++) {
              for (size_t k = 0; k < std::get<1>(regionSize); k++) {
                array(std::get<0>(outputRegionStart) + j,
                      std::get<1>(outputRegionStart) + k, imagePos) =
                    static_cast<ValueType>(
                        data(std::get<0>(inputRegionStart) + j,
                             std::get<1>(outputRegionStart) + k, 0));
              }
            }
          });

      imagePos++;
    }

    vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> version(
        dbusClientRef, HANDLEDBUSPENDINGREPLY(update->Finish(
                           dbusClientRef.clientPath(), vx::emptyOptions())));
    return version->versionString();
  } catch (Exception& e) {
    throw;
  } catch (std::exception& e) {
    throw Exception("de.uni_stuttgart.Voxie.HDFImporter.Error",
                    QString() + "Failure while loading image: " + e.what());
  }
}

DataTypeExt convertHDF5TypeToDataType(HDF5::DataType type) {
  // TODO: Clean up?
  if (type.getClass() == H5T_FLOAT) {
    switch (type.getSize()) {
      case 8:
        return DataTypeExt(BaseType::Float, 64, Endianness::Native);
      case 4:
        return DataTypeExt(BaseType::Float, 32, Endianness::Native);
      case 2:
        return DataTypeExt(BaseType::Float, 16, Endianness::Native);
    }
  } else if (type.getClass() == H5T_INTEGER) {
    if (type.getSign() == H5T_SGN_NONE) {
      switch (type.getSize()) {
        case 8:
          return DataTypeExt(BaseType::UInt, 64, Endianness::Native);
        case 4:
          return DataTypeExt(BaseType::UInt, 32, Endianness::Native);
        case 2:
          return DataTypeExt(BaseType::UInt, 16, Endianness::Native);
        case 1:
          return DataTypeExt(BaseType::UInt, 8, Endianness::None);
      }
    } else {
      switch (type.getSize()) {
        case 8:
          return DataTypeExt(BaseType::Int, 64, Endianness::Native);
        case 4:
          return DataTypeExt(BaseType::Int, 32, Endianness::Native);
        case 2:
          return DataTypeExt(BaseType::Int, 16, Endianness::Native);
        case 1:
          return DataTypeExt(BaseType::Int, 8, Endianness::None);
      }
    }
  } else if (type.getClass() == H5T_ENUM) {
    // auto baseType = type.getSuper();
    auto typeEnum = (HDF5::EnumType)type;
    // qDebug() << "typeEnum.nMembers()" << typeEnum.nMembers();
    if (typeEnum.nMembers() == 2) {
      auto mem0 = typeEnum.memberName(0);
      auto mem1 = typeEnum.memberName(1);
      /*
      qDebug() << "member names" << QString::fromStdString(mem0)
               << QString::fromStdString(mem1);
      */
      // TODO: Allow other enum names for true and false? Would also require
      // changes in BoolHdf5.cpp (and maybe some way to make sure the "correct"
      // DataType will be used).
      if ((mem0 == "FALSE" && mem1 == "TRUE") ||
          (mem0 == "TRUE" && mem1 == "FALSE"))
        return DataTypeExt(BaseType::Bool, 8, Endianness::None);
    }
  }
  qWarning() << "Unknown HDF5::DataType. Falling back to DataType::Float32.";
  return DataTypeExt(BaseType::Float, 32, Endianness::Native);
}
