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

#include "VolumeDataVoxelInst.hpp"

using namespace vx;

template <class T>
class TransformWorker : public QRunnable {
 private:
  QSharedPointer<VolumeDataVoxel> dataSet;
  size_t slice;

  const std::function<T(T)>& f;

 public:
  TransformWorker(const QSharedPointer<VolumeDataVoxel>& dataSet, size_t slice,
                  const std::function<T(T)>& f)
      : QRunnable(), dataSet(dataSet), slice(slice), f(f) {
    this->setAutoDelete(true);
  }

  virtual void run() override {
    auto data =
        qSharedPointerDynamicCast<VolumeDataVoxelInst<T>>(this->dataSet);
    if (!data) {
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "TransformWorker::run(): Cannot cast volume");
    }

    for (size_t y = 0; y < data->getDimensions().y; y++) {
      for (size_t x = 0; x < data->getDimensions().x; x++) {
        data->setVoxel(x, y, this->slice,
                       this->f(data->getVoxel(x, y, this->slice)));
      }
    }
  }
};

template <class T>
class CoordinatedTransformWorker : public QRunnable {
 private:
  QSharedPointer<VolumeDataVoxel> dataSet;
  size_t slice;
  const std::function<T(size_t, size_t, size_t, T)>& f;

 public:
  CoordinatedTransformWorker(
      const QSharedPointer<VolumeDataVoxel>& dataSet, size_t slice,
      const std::function<T(size_t, size_t, size_t, T)>& f)
      : QRunnable(), dataSet(dataSet), slice(slice), f(f) {
    this->setAutoDelete(true);
  }

  virtual void run() override {
    auto data =
        qSharedPointerDynamicCast<VolumeDataVoxelInst<T>>(this->dataSet);
    if (!data) {
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InternalError",
          "CoordinatedTransformWorker::run(): Cannot cast volume");
    }

    for (size_t y = 0; y < data->getDimensions().y; y++) {
      for (size_t x = 0; x < data->getDimensions().x; x++) {
        data->setVoxel(
            x, y, this->slice,
            this->f(x, y, this->slice, data->getVoxel(x, y, this->slice)));
      }
    }
  }
};

template <class T>
VolumeDataVoxelInst<T>::VolumeDataVoxelInst(size_t width, size_t height,
                                            size_t depth, DataType dataType)
    : VolumeDataVoxel(width, height, depth, dataType) {
  this->dataType = dataType;
}

template <class T>
void VolumeDataVoxelInst<T>::transform(const std::function<T(T)>& f) {
  auto thisPointer = this->thisShared();
  QThreadPool pool;
  for (size_t z = 0; z < this->dimensions.z; z++) {
    pool.start(new TransformWorker<T>(thisPointer, z, f));
  }
  pool.waitForDone();
  this->invalidate();
}

template <class T>
void VolumeDataVoxelInst<T>::transformCoordinate(
    const std::function<T(size_t, size_t, size_t, T)>& f) {
  auto thisPointer = this->thisShared();
  QThreadPool pool;
  for (size_t z = 0; z < this->dimensions.z; z++) {
    pool.start(new CoordinatedTransformWorker<T>(thisPointer, z, f));
  }
  pool.waitForDone();
  this->invalidate();
}

template <class T>
void VolumeDataVoxelInst<T>::transformCoordinateSingledThreaded(
    const std::function<T(size_t, size_t, size_t, T)>& f) {
  for (size_t z = 0; z < this->dimensions.z; z++) {
    for (size_t y = 0; y < this->dimensions.y; y++) {
      for (size_t x = 0; x < this->dimensions.x; x++) {
        this->setVoxel(x, y, z, f(x, y, z, this->getVoxel(x, y, z)));
      }
    }
  }
  this->invalidate();
}

static inline qint64 castToInt(qreal f, bool* overflow) {
  f = std::floor(f);
  // TODO: There are rounding errors during the conversion from qint64 to float,
  // so that e.g. 9223372036854775807 is converted incorrectly.
  if (f < std::numeric_limits<qint64>::min() ||
      f > std::numeric_limits<qint64>::max()) {
    if (overflow) *overflow = true;
    return 0;
  }
  return (qint64)f;
}

// TODO: Warning suppressed
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
template <class GenericVoxel>
GenericVoxel
VolumeDataVoxelInst<GenericVoxel>::VolumeDataVoxelInst::getVoxelMetric(
    qreal x, qreal y, qreal z, InterpolationMethod method) const {
  // transform to 'integer' coordinatesystem
  x -= origin().x();
  y -= origin().y();
  z -= origin().z();
  x /= this->spacing.x();
  y /= this->spacing.y();
  z /= this->spacing.z();
  // qDebug() << "getVoxelMetric 2" << x << y << z;

  if (method == vx::InterpolationMethod::NearestNeighbor) {
    if (x < 0 || y < 0 || z < 0) {
      return static_cast<GenericVoxel>(NAN);
    }
    // Round downwards
    // TODO: Handle overflow properly
    size_t xi = (size_t)std::floor(x);
    size_t yi = (size_t)std::floor(y);
    size_t zi = (size_t)std::floor(z);
    return getVoxelSafe(xi, yi, zi);
  }
  if (method == vx::InterpolationMethod::Linear) {
    x -= 0.5;
    y -= 0.5;
    z -= 0.5;
    bool overflow = false;
    auto xi = castToInt(x, &overflow);
    auto yi = castToInt(y, &overflow);
    auto zi = castToInt(z, &overflow);
    // When there was an overflow during conversion or all used voxels are out
    // of range, return NaN
    if (overflow || (xi < -1) || (yi < -1) || (xi < -1) ||
        ((size_t)xi >= dimensions.x && ((size_t)xi + 1) >= dimensions.x) ||
        ((size_t)yi >= dimensions.y && ((size_t)yi + 1) >= dimensions.y) ||
        ((size_t)zi >= dimensions.z && ((size_t)zi + 1) >= dimensions.z))
      return static_cast<GenericVoxel>(NAN);
    // coefficients
    qreal kx = x - xi;
    qreal ky = y - yi;
    qreal kz = z - zi;
    // if (x < 0 || y < 0 || z < 0)
    // qDebug() << x << y << z << xi << yi << zi << kx << ky << kz << overflow;
    // Use defVal = 0 to make sure a proper value is returned if the position is
    // at least one of the 8 used voxels is in the volume
    GenericVoxel defVal = static_cast<GenericVoxel>(0);
    GenericVoxel vox = static_cast<GenericVoxel>(
        kx * ky * kz * getVoxelSafe(xi + 1, yi + 1, zi + 1, defVal) +
        kx * ky * (1 - kz) * getVoxelSafe(xi + 1, yi + 1, zi, defVal) +
        kx * (1 - ky) * kz * getVoxelSafe(xi + 1, yi, zi + 1, defVal) +
        kx * (1 - ky) * (1 - kz) * getVoxelSafe(xi + 1, yi, zi, defVal) +
        (1 - kx) * ky * kz * getVoxelSafe(xi, yi + 1, zi + 1, defVal) +
        (1 - kx) * ky * (1 - kz) * getVoxelSafe(xi, yi + 1, zi, defVal) +
        (1 - kx) * (1 - ky) * kz * getVoxelSafe(xi, yi, zi + 1, defVal) +
        (1 - kx) * (1 - ky) * (1 - kz) * getVoxelSafe(xi, yi, zi, defVal));
    return vox;
  }
  return static_cast<GenericVoxel>(NAN);
}
// TODO: Warning suppressed
#pragma GCC diagnostic pop

template <class GenericVoxel>
GenericVoxel
VolumeDataVoxelInst<GenericVoxel>::VolumeDataVoxelInst::getVoxelMetric(
    QVector3D position, InterpolationMethod method) const {
  return this->getVoxelMetric(position.x(), position.y(), position.z(), method);
}

template <class T>
QSharedPointer<VolumeDataVoxel> VolumeDataVoxelInst<T>::reducedSize(
    uint xStepsize, uint yStepsize, uint zStepsize) const {
  xStepsize = xStepsize == 0 ? 1 : xStepsize;
  yStepsize = yStepsize == 0 ? 1 : yStepsize;
  zStepsize = zStepsize == 0 ? 1 : zStepsize;

  auto currentDims = this->dimensions;
  vx::VectorSizeT3 newDims((currentDims.x + xStepsize - 1) / xStepsize,
                           (currentDims.y + yStepsize - 1) / yStepsize,
                           (currentDims.z + zStepsize - 1) / zStepsize);

  auto newVolumeDataVoxel =
      createVolume(newDims.x, newDims.y, newDims.z, dataType);

  QVector3D spacing =
      this->spacing * QVector3D(xStepsize, yStepsize, zStepsize);
  newVolumeDataVoxel->setSpacing(spacing);
  newVolumeDataVoxel->setOrigin(this->origin());

  auto newVolumeDataVoxelInst =
      qSharedPointerDynamicCast<VolumeDataVoxelInst<T>>(newVolumeDataVoxel);
  if (!newVolumeDataVoxelInst) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "VolumeDataVoxelInst<T>::reducedSize(): Cannot cast volume");
  }

  for (size_t z = 0; z < newDims.z; z++) {
    for (size_t y = 0; y < newDims.y; y++) {
      for (size_t x = 0; x < newDims.x; x++) {
        T vox = this->getVoxel(x * xStepsize, y * yStepsize, z * zStepsize);
        newVolumeDataVoxelInst->setVoxel(x, y, z, vox);
      }
    }
  }

  return newVolumeDataVoxel;
}

template <class T>
cl::Image3D& VolumeDataVoxelInst<T>::getCLImage() {
  if (!this->isImageInit) {
    try {
      switch (this->getDataType()) {
        case DataType::Float16:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_HALF_FLOAT), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::Float32:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_FLOAT), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::Int8:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_SIGNED_INT8), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::Int16:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_SIGNED_INT16), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::Int32:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_SIGNED_INT32), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::UInt8:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_UNSIGNED_INT8), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::UInt16:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_UNSIGNED_INT16), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        case DataType::UInt32:
          this->clImage =
              vx::opencl::CLInstance::getDefaultInstance()->createImage3D(
                  cl::ImageFormat(CL_R, CL_UNSIGNED_INT32), this->dimensions.x,
                  this->dimensions.y, this->dimensions.z, this->getData());
          break;
        default:
          this->clImage = cl::Image3D();
          qWarning() << "Error when allocing OpenCL image for voxel data: "
                        "openCL does not support the specified data type"
                     << vx::getDataTypeString(this->getDataType());
          break;
      }
    } catch (vx::opencl::CLException& ex) {
      this->clImage = cl::Image3D();
      qWarning() << "Error when allocing OpenCL image for voxel data:" << ex;
    }
    // Set isImageInit even if allocating the image failed. In this case,
    // clImage is null and there will be no further attempt to allocate
    // the image.
    this->isImageInit = this->clImageValid = true;
  }
  if (!this->clImageValid) {
    updateClImage();
  }
  return this->clImage;
}

template <class T>
void VolumeDataVoxelInst<T>::updateClImage() {
  if (isImageInit && clImage() != nullptr) {
    try {
      vx::opencl::CLInstance::getDefaultInstance()->fillImage(&this->clImage,
                                                              this->getData());
      this->clImageValid = true;
      // qDebug() << "climage update";
    } catch (vx::opencl::CLException& ex) {
      qWarning() << ex;
    }
  }
}

QSharedPointer<VolumeDataVoxel> VolumeDataVoxel::createVolume(
    size_t width, size_t height, size_t depth, DataType dataTypeInput) {
  return switchOverDataType<VolumeDataVoxel::SupportedTypes,
                            QSharedPointer<VolumeDataVoxel>>(
      dataTypeInput, [&](auto traits) {
        using T = typename decltype(traits)::Type;
        return createBase<VolumeDataVoxelInst<T>>(width, height, depth,
                                                  dataTypeInput);
      });
}

template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<half_float::half>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<float>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<double>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<int8_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<int16_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<int32_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<int64_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<uint8_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<uint16_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<uint32_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<uint64_t>;
template class VOXIEBACKEND_EXPORT VolumeDataVoxelInst<vx::Bool8>;

// template<class T> void VolumeDataVoxelInst<T>::callFunction(std::function<
// void(auto& self) >& lambda){
//    lambda(this);
//}
