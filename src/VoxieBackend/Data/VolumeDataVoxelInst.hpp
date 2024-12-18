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

#include <VoxieClient/Bool8.hpp>

#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VoxelAccessor.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>

#include <half.hpp>

struct vx::VolumeDataVoxel::SupportedTypes
    : vx::DataTypeList<
          vx::DataType::Float16, vx::DataType::Float32, vx::DataType::Float64,
          vx::DataType::Int8, vx::DataType::Int16, vx::DataType::Int32,
          vx::DataType::Int64, vx::DataType::UInt8, vx::DataType::UInt16,
          vx::DataType::UInt32, vx::DataType::UInt64, vx::DataType::Bool8> {};

template <class T>
class VOXIEBACKEND_EXPORT VolumeDataVoxelInst : public vx::VolumeDataVoxel {
  typedef T GenericVoxel;

 public:
  using ValueType = T;
  using VoxelType = T;  // TODO: Remove

  class Accessor : public vx::VoxelAccessor<Accessor, T> {
    QSharedPointer<VolumeDataVoxelInst> object;

   public:
    Accessor(QSharedPointer<VolumeDataVoxelInst> object)
        : object(std::move(object)) {}

    // TODO: Copy values for performance?
    const vx::Vector<size_t, 3>& arrayShape() const {
      return object->arrayShape();
    }
    const vx::Vector<double, 3>& volumeOrigin() const {
      return object->volumeOrigin();
    }
    const vx::Vector<double, 3>& gridSpacing() const {
      return object->gridSpacing();
    }

    inline T getVoxelUnchecked(const vx::Vector<size_t, 3>& pos) const {
      return object->getVoxelUnchecked(pos);
    }
  };

 private:
  QPair<GenericVoxel, GenericVoxel> minMax;

  // Returns the default value for getVoxelSafe() outside the image
  // TODO: Shouldn't getVoxelSafe() (or at least getVoxelMetric()) return a
  // floating point value?
  // TODO: Clean up getVoxelSafe() etc., return an Optional<> value instead of
  // nan for outside the image, unify code with VolumeDataBlock code.
  static GenericVoxel defaultValue() {
    return std::numeric_limits<GenericVoxel>::has_quiet_NaN
               ? std::numeric_limits<GenericVoxel>::quiet_NaN()
               : std::numeric_limits<GenericVoxel>::min();
  }

 public:
  VolumeDataVoxelInst(const vx::Vector<size_t, 3>& arrayShape,
                      vx::DataType dataType,
                      const vx::Vector<double, 3>& gridOrigin,
                      const vx::Vector<double, 3>& gridSpacing);

  Accessor accessor() {
    return Accessor(
        qSharedPointerDynamicCast<VolumeDataVoxelInst>(thisShared()));
  }

  /**
   * @brief Transform all voxels of the data set inplace.
   * @param f The function that transforms the voxels.
   * @remarks The transformation runs with thread pooling.
   */
  void transform(const std::function<GenericVoxel(GenericVoxel)>& f);

  /**
   * @brief Transform all voxels of the data set inplace with given voxel
   * coordinates.
   * @param f The function that transforms the voxels.
   * @remarks The transformation runs with thread pooling.
   */
  void transformCoordinate(
      const std::function<GenericVoxel(size_t, size_t, size_t, GenericVoxel)>&
          f);

  void transformCoordinateSingledThreaded(
      const std::function<GenericVoxel(size_t, size_t, size_t, GenericVoxel)>&
          f);

  /**
   * updates clImage of this dataset. need to call this if clImage is not
   * valid due to changes to the datasets data.
   * @see isClImageValid()
   */
  void updateClImage();

  cl::Image3D& getCLImage();

  // void callFunction(std::function< void(auto& self) >& lambda)
  // override;

  /**
   * @return calculates min and max of the values in this dataset
   * and retunspair(min,max). This is an expensive operation
   * @see getMinMaxValue()
   */
  inline QPair<GenericVoxel, GenericVoxel> calcMinMaxValue() {
    // qDebug() << "calcminmax";
    GenericVoxel min = (GenericVoxel)(std::numeric_limits<GenericVoxel>::max());
    GenericVoxel max = std::numeric_limits<GenericVoxel>::lowest();
    GenericVoxel* values = (GenericVoxel*)this->getData();
    for (size_t i = 0; i < this->size; i++) {
      if (min > values[i]) min = values[i];
      if (max < values[i]) max = values[i];
    }
    return QPair<GenericVoxel, GenericVoxel>(min, max);
  }

  /**
   * @return cached pair(min,max) of the values in this dataset. Calls
   * calcMinMax on demand when changes have been made to the data and
   * cached minmax is no longer valid.
   */
  inline const QPair<GenericVoxel, GenericVoxel>& getMinMaxValue() {
    if (!this->minMaxValid) {
      this->minMax = calcMinMaxValue();
      this->minMaxValid = true;
    }
    return this->minMax;
  }

  /**
   * @return cached pair(min,max) of the values in this dataset.
   * @param valid is set to true when the returned value is valid. this may
   * not be the case when changes have been made to this datasets data.
   */
  const QPair<GenericVoxel, GenericVoxel>& getMinMaxValueConst(
      bool* valid = nullptr) {
    if (valid != nullptr) *valid = this->minMaxValid;
    return this->minMax;
  }

  /**
   * @return whether cached minmax values are valid
   */
  bool isMinMaxValid() { return this->minMaxValid; }

  /**
   * @return voxel value at the given position. x,y,z outside the dimensions
   * will cause a crash.
   */
  inline GenericVoxel getVoxel(size_t x, size_t y, size_t z) {
    return this->getData()[x + y * this->arrayShape().template access<0>() +
                           z * this->arrayShape().template access<0>() *
                               this->arrayShape().template access<1>()];
  }
  inline GenericVoxel getVoxelUnchecked(const vx::Vector<size_t, 3>& pos) {
    return this
        ->getData()[pos[0] + pos[1] * this->arrayShape().template access<0>() +
                    pos[2] * this->arrayShape().template access<0>() *
                        this->arrayShape().template access<1>()];
  }

  /**
   * @return voxel value at the given position. x,y,z outside the dimensions
   * will return NAN / minValue (for integers).
   */
  inline GenericVoxel getVoxelSafe(size_t x, size_t y, size_t z,
                                   GenericVoxel def = defaultValue()) {
    if (isInBounds(x, y, z, this->arrayShape())) {
      return this->getData()[x + y * this->arrayShape().template access<0>() +
                             z * this->arrayShape().template access<0>() *
                                 this->arrayShape().template access<1>()];
    } else {
      return def;
    }
  }

  /**
   * @return voxel value at the given position. pos outside the dimensions will
   * cause a crash.
   */
  inline GenericVoxel getVoxel(const vx::VectorSizeT3& pos) {
    return getVoxel(pos.x, pos.y, pos.z);
  }

  /**
   * @return voxel value at the given position. pos outside the dimensions
   * will return NAN.
   */

// TODO: Warning suppressed
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
#endif
  inline virtual GenericVoxel getVoxelSafe(const vx::VectorSizeT3& pos) {
    return getVoxelSafe(pos.x, pos.y, pos.z);
  }
// TODO: Warning suppressed
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  // TODO: Remove getVoxelMetric() methods and replace them with VoxelAccessor,
  // or at least use the same implementation?

  /**
   * @param method the interpolation method: NearestNeighbor or Linear
   * (=default)
   * @return voxel value at the interpolated metric position.
   */
  GenericVoxel getVoxelMetric(
      qreal x, qreal y, qreal z,
      vx::InterpolationMethod method = vx::InterpolationMethod::Linear);

  GenericVoxel getVoxelMetric(
      QVector3D position,
      vx::InterpolationMethod method = vx::InterpolationMethod::Linear);

  GenericVoxel getVoxelMetric(
      const vx::Vector<double, 3>& position,
      vx::InterpolationMethod method = vx::InterpolationMethod::Linear);

  /**
   * @brief setVoxel sets the value of the voxel at the given position to voxel.
   * x,y,z outside the dimensions will crash.
   */
  inline void setVoxel(size_t x, size_t y, size_t z, GenericVoxel voxel) {
    this->getData()[x + y * this->arrayShape().template access<0>() +
                    z * this->arrayShape().template access<0>() *
                        this->arrayShape().template access<1>()] = voxel;
  }

  /**
   * @brief setVoxel sets the value of the voxel at the given position to voxel.
   * x,y,z outside the dimensions will do nothing.
   */
  inline void setVoxelSafe(size_t x, size_t y, size_t z, GenericVoxel voxel) {
    if (isInBounds(x, y, z, this->arrayShape())) {
      this->getData()[x + y * this->arrayShape().template access<0>() +
                      z * this->arrayShape().template access<0>() *
                          this->arrayShape().template access<1>()] = voxel;
    }
  }

  /**
   * @brief setVoxel sets the value of the voxel at the given position to voxel.
   * pos outside the dimensions will crash.
   */
  inline void setVoxel(const vx::VectorSizeT3& pos, GenericVoxel voxel) {
    setVoxel(pos.x, pos.y, pos.z, voxel);
  }

  /**
   * @brief setVoxel sets the value of the voxel at the given position to voxel.
   * pos outside the dimensions will crash.
   */
  inline void setVoxelSafe(const vx::VectorSizeT3& pos, GenericVoxel voxel) {
    setVoxel(pos.x, pos.y, pos.z, voxel);
  }

  /**
   * @return voxel data as an array; x increases fastest, then y, then z.
   */
  inline GenericVoxel* getData() {
    return (GenericVoxel*)this->dataSH->getData();
  }

  /**
   * @return size of the voxel data set in bytes.
   */
  inline size_t getByteSize() { return this->size * sizeof(GenericVoxel); }

  /**
   * @return size of one generic voxel in bytes.
   */
  inline size_t getSingleVoxelByteSize() { return sizeof(GenericVoxel); }

  QSharedPointer<VolumeDataVoxel> reducedSize(uint xStepsize = 2,
                                              uint yStepsize = 2,
                                              uint zStepsize = 2);

  vx::Array3<void> getBlockVoid(const vx::Vector<size_t, 3> offset,
                                const vx::Vector<size_t, 3> size) override;
  vx::Array3<T> getBlock(const vx::Vector<size_t, 3> offset,
                         const vx::Vector<size_t, 3> size);
};

namespace vx {
template <typename List, typename F, typename Ret>
Ret VolumeDataVoxel::performInGenericContextRestricted(const F& lambda) {
  // TODO: Check whether all types in List are also in
  // VolumeDataVoxel::SupportedTypes?
  return switchOverDataType<List, Ret>(this->getDataType(), [&](auto traits) {
    using T = typename decltype(traits)::Type;
    return lambda((VolumeDataVoxelInst<T>&)(*this));
  });
}
template <typename F, typename Ret>
Ret VolumeDataVoxel::performInGenericContext(const F& lambda) {
  return this->performInGenericContextRestricted<
      VolumeDataVoxel::SupportedTypes, F, Ret>(lambda);
}
}  // namespace vx
