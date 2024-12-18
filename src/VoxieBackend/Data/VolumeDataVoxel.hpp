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

#include <Voxie/MathQt.hpp>

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ArrayInfo.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/InterpolationMethod.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>
#include <VoxieBackend/Data/VolumeData.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>
#include <VoxieBackend/VoxieBackend.hpp>

#include <cmath>
#include <functional>

#include <inttypes.h>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QTime>
#include <QtCore/QVariant>
#include <QtCore/QtGlobal>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusUnixFileDescriptor>

#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>

template <class T>
class VOXIEBACKEND_EXPORT VolumeDataVoxelInst;

/**
 * @ingroup data
 * @author Andreas Korge
 * @version 1.0
 */
namespace vx {

// forward declaration
class HistogramProvider;

/**
 * The VolumeDataVoxel class is a 3 dimensional dataset (image) that contains
 * float values.
 * @brief The VolumeDataVoxel class
 */
class VOXIEBACKEND_EXPORT VolumeDataVoxel : public VolumeData {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  struct SupportedTypes;

 private:
 protected:
  cl::Image3D clImage;
  bool isImageInit = false;
  bool clImageValid = false;
  QFileInfo fileInfo;

  size_t size;
  vx::Vector<size_t, 3> arrayShape_;
  vx::Vector<double, 3> gridSpacing_;
  DataType dataType = DataType::Float32;

  QMap<quint32, QWeakPointer<vx::HistogramProvider>> histogramProviders;

  QSharedPointer<SharedMemory> dataSH;

  bool minMaxValid = false;

  bool isInBounds(size_t x, size_t y, size_t z,
                  const vx::Vector<size_t, 3>& arrayShape) const {
    return x < arrayShape.access<0>() && y < arrayShape.access<1>() &&
           z < arrayShape.access<2>();
  }

  // throws Exception
  VolumeDataVoxel(const vx::Vector<size_t, 3>& arrayShape, DataType dataType,
                  const vx::Vector<double, 3>& volumeOrigin,
                  const vx::Vector<double, 3>& gridSpacing);

 public:
  // throws Exception
  // Can also be called from non-main thread (object will be created on main
  // thread)

  // Implementation is in VolumeDataVoxelInst.cpp
  static QSharedPointer<VolumeDataVoxel> createVolume(
      const vx::Vector<size_t, 3>& arrayShape, DataType dataType,
      const vx::Vector<double, 3>& gridOrigin,
      const vx::Vector<double, 3>& gridSpacing);

  ~VolumeDataVoxel();

  QList<QString> supportedDBusInterfaces() override;

  template <typename F, typename Ret = decltype((*(F*)nullptr)(
                            (*(VolumeDataVoxelInst<float>*)nullptr)))>
  Ret performInGenericContext(const F& lambda);

  template <typename List, typename F,
            typename Ret = decltype((*(F*)nullptr)(
                (*(VolumeDataVoxelInst<float>*)nullptr)))>
  Ret performInGenericContextRestricted(const F& lambda);

  // void callFunction(std::function < void(VolumeDataVoxelInst<>& self) >&
  // lambda) = 0;

  /**
   * @return number of Voxels in dataset
   */
  // TODO: rename
  size_t getSize() const { return this->size; }

  /**
   * @return x, y, z dimensions of the dataset
   */
  const vx::Vector<size_t, 3>& arrayShape() { return arrayShape_; }
  // TODO: Remove
  vx::VectorSizeT3 getDimensions() { return arrayShape(); }

  /**
   * @return spacing/scaling of the dataset.
   */
  const vx::Vector<double, 3>& gridSpacing() { return gridSpacing_; }
  // TODO: Remove
  QVector3D getSpacing() {
    return toQVector(vectorCastNarrow<float>(this->gridSpacing()));
  }

  DataType getDataType() override { return this->dataType; }

  /**
   * @brief Calculates voxel to 3D [m] transformation matrix (Affine-Trafo:
   * Rot+scaling).
   * @return voxel to 3D [m] transformation matrix
   */
  QMatrix4x4 getVoxelTo3DTrafo() {
    QMatrix4x4 voxelTo3D;
    voxelTo3D.translate(this->origin());
    voxelTo3D.scale(this->getSpacing());
    return voxelTo3D;
  }

  /**
   * @brief Calculates voxel to object [m] coordinate system transformation
   * matrix (Trans+Scale)
   * @return transformation matrix
   */
  AffineMap<double, 3, 3> getVoxelToObjectTrafo();

  AffineMap<double, 3, 3> getObjectToVoxelTrafo();

  // throws Exception
  QSharedPointer<VolumeDataVoxel> reducedSize(uint xStepsize = 2,
                                              uint yStepsize = 2,
                                              uint zStepsize = 2) const;

  /**
   * @return  whether climage of this dataset is valid
   * (in sync with current data)
   */
  bool isClImageValid() const { return this->clImageValid; }

  /**
   * Creates a histogram provider with the specified bucket count.
   *
   * HistogramProvider instances are cached as weak pointers and reused for
   * matching bucket counts.
   */
  QSharedPointer<vx::HistogramProvider> getHistogramProvider(
      quint32 bucketCount);

 public:
  vx::Array3Info dataFd(bool rw);

 public Q_SLOTS:
  /**
   * meant to be called by code that makes changes to
   * this datasets data. Sets minMaxValid and clImageValid to false
   */
  void invalidate() { this->minMaxValid = this->clImageValid = false; }

 public:
  // throws Exception
  void extractSlice(const QVector3D& origin, const QQuaternion& rotation,
                    const QSize& outputSize, double pixelSizeX,
                    double pixelSizeY, InterpolationMethod interpolation,
                    FloatImage& outputImage) override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

  void getVolumeInfo(QList<std::tuple<QString, QString>>& fields) override;

  double getStepSize(const vx::Vector<double, 3>& dir) override;

 private:
  void updateHistogram(QSharedPointer<vx::HistogramProvider> histogramProvider,
                       quint32 bucketCount);

  void updateAllHistograms();

 public:
  template <typename Callback>
  void forEachVoxelImpl(const Callback& callback) {
    this->performInGenericContext([&](auto& data) {
      auto dataPtr = data.getData();
      auto dim = this->getDimensions();
      const vx::Vector<double, 3> origin = this->volumeOrigin();
      const vx::Vector<double, 3> spacing = this->gridSpacing();

      for (size_t z = 0; z < dim.z; z++) {
        const double posZ = origin.access<2>() + z * spacing.access<2>();
        for (size_t y = 0; y < dim.y; y++) {
          const double posY = origin.access<1>() + y * spacing.access<1>();
          for (size_t x = 0; x < dim.x; x++) {
            const double posX = origin.access<0>() + x * spacing.access<0>();
            // TODO: Allow modifying values?
            // Probably should be another overload which makes sure a DataUpdate
            // exists
            const auto value = dataPtr[x + y * dim.x + z * dim.x * dim.y];
            callback(value, vx::Vector<double, 3>{posX, posY, posZ}, spacing);
          }
        }
      }
    });
  }

  void extractGrid(const QVector3D& origin, const QQuaternion& rotation,
                   const QSize& outputSize, double pixelSizeX,
                   double pixelSizeY, QImage& outputImage, QRgb color) override;

  virtual vx::Array3<void> getBlockVoid(const vx::Vector<size_t, 3> offset,
                                        const vx::Vector<size_t, 3> size) = 0;
};
}  // namespace vx
