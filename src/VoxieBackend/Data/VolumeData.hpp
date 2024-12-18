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

#include <VoxieClient/Vector.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/InterpolationMethod.hpp>

// TODO: Don't use this here
#include <Voxie/MathQt.hpp>

#include <QtDBus/QDBusAbstractAdaptor>

#include <QtGui/QColor>
#include <QtGui/QVector3D>

namespace vx {

class FloatImage;
class VolumeStructure;

/**
 * The VolumeData class is the base class for storing a 3 dimensional dataset
 * (image).
 */
class VOXIEBACKEND_EXPORT VolumeData : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 private:
  vx::Vector<double, 3> volumeOrigin_;
  vx::Vector<double, 3> volumeSize_;
  QSharedPointer<VolumeStructure> volumeStructure_;

 protected:
  // throws Exception
  VolumeData(const vx::Vector<double, 3>& volumeOrigin,
             const vx::Vector<double, 3>& volumeSize,
             const QSharedPointer<VolumeStructure>& volumeStructure);

  ~VolumeData();

 public:
  /**
   * @return x, y, z dimensions in meters of the dataset
   */
  const vx::Vector<double, 3>& volumeSize() { return volumeSize_; }
  // TODO: Remove
  QVector3D getDimensionsMetric() {
    return toQVector(vectorCastNarrow<float>(this->volumeSize()));
  }

  /**
   * @return Position of lower left corner of the first voxel in meters.
   */
  const vx::Vector<double, 3>& volumeOrigin() const {
    return this->volumeOrigin_;
  }
  // TODO: Remove
  QVector3D origin() const {
    return toQVector(vectorCastNarrow<float>(this->volumeOrigin()));
  }

  const QSharedPointer<VolumeStructure>& volumeStructure() const {
    return this->volumeStructure_;
  }

 Q_SIGNALS:
  void changed();  // TODO: remove, replace by Data::dataChanged?

 public:
  // TODO: Move this somewhere else? VolumeDataBlock does not really have a data
  // type.
  virtual DataType getDataType() = 0;

  // throws Exception
  virtual void extractSlice(const QVector3D& origin,
                            const QQuaternion& rotation,
                            const QSize& outputSize, double pixelSizeX,
                            double pixelSizeY,
                            InterpolationMethod interpolation,
                            FloatImage& outputImage) = 0;

  /**
   * Return key/value pairs continaing information about the volume.
   *
   * When the information changes, the dataChanged event will be emitted.
   */
  virtual void getVolumeInfo(QList<std::tuple<QString, QString>>& fields) = 0;

  /**
   * Return the size of one voxel (or some estimate for non-voxel datasets) in
   * direction dir.
   */
  virtual double getStepSize(const vx::Vector<double, 3>& dir) = 0;

  /**
   * Call callback with this cast to the actual VolumeData type (e.g.
   * VolumeDataVoxel) as a QSharedPointer.
   *
   * Using this function requires including
   * <VoxieBackend/Data/VolumeDataInst.hpp>
   */
  template <typename Callback>
  inline void withVolumeType(const Callback& callback);

  /**
   * Call callback for each voxel in the volume with the the value of the voxel,
   * the lower left position of the voxel and the size of the voxel.
   *
   * Using this function requires including
   * <VoxieBackend/Data/VolumeDataInst.hpp>
   */
  template <typename Callback>
  void forEachVoxel(const Callback& callback) {
    this->withVolumeType(
        [&](const auto& self) { self->forEachVoxelImpl(callback); });
  }

  virtual void extractGrid(const QVector3D& origin, const QQuaternion& rotation,
                           const QSize& outputSize, double pixelSizeX,
                           double pixelSizeY, QImage& outputImage,
                           QRgb color) = 0;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::VolumeData*)
