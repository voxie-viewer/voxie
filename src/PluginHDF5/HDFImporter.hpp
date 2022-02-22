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

#pragma once

#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>

#include <PluginHDF5/CT/DataFiles.hpp>
#include <PluginHDF5/CT/rawdatafiles.hpp>
#include <PluginHDF5/CT/typefiles.hpp>

#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/IO/Operation.hpp>

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItemIterator>

#include <hdf5.h>
#include <VoxieBackend/IO/Importer.hpp>

using namespace vx;

class
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 7
    // Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80947
    // https://stackoverflow.com/questions/44390898/gcc-6-x-warning-about-lambda-visibility
    // Needed on Debian stretch / gcc (Debian 6.3.0-18+deb9u1) 6.3.0 20170516
    // Not needed on Ubuntu bionic / gcc (Ubuntu 7.4.0-1ubuntu1~18.04.1) 7.4.0
    __attribute__((visibility("default")))
#endif
    HDFImporter : public vx::io::Importer {
  Q_OBJECT
  Q_DISABLE_COPY(HDFImporter)

 public:
  static QVector3D toQVector(Math::Vector3<double> vec) {
    return QVector3D(vec.x(), vec.y(), vec.z());
  }
  static QVector3D toQVector(Math::DiagMatrix3<double> vec) {
    return QVector3D(vec.m11(), vec.m22(), vec.m33());
  }

  static DataType convertHDF5TypeToDataType(HDF5::DataType type);

 private:
  // Can throw arbitrary exceptions
  static QSharedPointer<Data> loadVoxelData(
      const HDF5::File& file, const QSharedPointer<vx::io::Operation>& op);
  // Can throw arbitrary exceptions
  static QSharedPointer<Data> loadRawData(
      const HDF5::File& file, const QSharedPointer<vx::io::Operation>& op);

  template <class Type>
  static QSharedPointer<VolumeDataVoxel> createGenericVolume(
      HDF5::File file, DataType typeToUse,
      const QSharedPointer<vx::io::Operation>& op) {
    QSharedPointer<VolumeDataVoxel> voxelData;

    std::shared_ptr<VolumeGen<Type, true>> volume =
        HDF5::matlabDeserialize<VolumeGen<Type, true>>(file);
    Math::Vector3<size_t> size = getSize(*volume);
    // create and fill the voxel data object
    voxelData =
        VolumeDataVoxel::createVolume(size.x(), size.y(), size.z(), typeToUse);

    voxelData->performInGenericContext([&](auto& data) {
      size_t shape[3] = {size.x(), size.y(), size.z()};

      ptrdiff_t stridesBytes[3] = {
          (ptrdiff_t)(sizeof(Type)), (ptrdiff_t)(size.x() * sizeof(Type)),
          (ptrdiff_t)(size.x() * size.y() * sizeof(Type))};
      Math::ArrayView<Type, 3> view((Type*)data.getData(), shape, stridesBytes);
      /*should be float even if half or integer type is used for volume data*/
      loadAndTransformTo<Type>(*volume, view, [op](size_t pos, size_t count) {
        op->throwIfCancelled();
        op->updateProgress(1.0 * pos / count);
      });

      // read meta data
      if (volume->GridOrigin)
        voxelData->setOrigin(toQVector(*volume->GridOrigin));
      else
        voxelData->setOrigin(QVector3D(0, 0, 0));
      if (volume->GridSpacing)
        voxelData->setSpacing(toQVector(*volume->GridSpacing));
      else
        voxelData->setSpacing(QVector3D(1, 1, 1));
    });

    return voxelData;
  }

 public:
  explicit HDFImporter();
  ~HDFImporter();

  QSharedPointer<vx::OperationResultImport> import(
      const QString& fileName, bool doCreateNode,
      const QMap<QString, QVariant>& properties) override;
};

class TomographyRawData2DAccessorHDF5 : public TomographyRawData2DAccessor {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(TomographyRawData2DAccessorHDF5)

  HDF5::File file;
  std::shared_ptr<RawGen<float, true>> raw;

  uint64_t numberOfImages_;
  QJsonObject metadata_;
  QList<QJsonObject> availableImageKinds_;
  QList<QString> availableStreams_;
  QList<QString> availableGeometryTypes_;
  vx::VectorSizeT2 imageSize_;
  std::vector<double> angles;
  QMap<uint64_t, QJsonObject> perImageMetadata_;

  TomographyRawData2DAccessorHDF5(const HDF5::File& file);
  virtual ~TomographyRawData2DAccessorHDF5();

  // Allow the ctor to be called using create()
  friend class vx::RefCountedObject;

 public:
  uint64_t numberOfImages(const QString& stream) override;
  QJsonObject metadata() const override { return metadata_; }
  QList<QJsonObject> availableImageKinds() const override {
    return availableImageKinds_;
  }
  QList<QString> availableStreams() override { return availableStreams_; }
  bool hasStream(const QString& stream) override { return stream == ""; }
  QList<QString> availableGeometryTypes() override {
    return availableGeometryTypes_;
  }
  vx::VectorSizeT2 imageSize(const QString& stream, uint64_t id) override;
  QJsonObject getPerImageMetadata(const QString& stream, uint64_t id) override;
  QJsonObject getGeometryData(const QString& geometryType) override;

  QString readImages(const QJsonObject& imageKind,
                     const QList<std::tuple<QString, quint64>>& images,
                     const std::tuple<quint64, quint64>& inputRegionStart,
                     const QSharedPointer<TomographyRawData2DRegular>& output,
                     qulonglong firstOutputImageId,
                     const std::tuple<quint64, quint64>& outputRegionStart,
                     const std::tuple<quint64, quint64>& regionSize,
                     bool allowIncompleteData = false) override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};
