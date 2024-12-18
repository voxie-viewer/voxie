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

#include "ImageDataPixelInst.hpp"

#include <VoxieBackend/Data/TomographyRawData2DRegularInst.hpp>

typedef vx::ImageDataPixelInst<float, 1> ImageDataPixelOld;  // TODO: remove

template <typename T, size_t componentCount_>
vx::ImageDataPixelInst<T, componentCount_>::ImageDataPixelInst(
    quint64 width, quint64 height, bool writableFromDBus, DataType dataType)
    : ImageDataPixel(dataType, writableFromDBus),
      dataSH(createQSharedPointer<SharedMemory>(width * height *
                                                componentCount_ * sizeof(T))),
      array_((std::array<T, componentCount_>*)dataSH->getData(),
             std::array<size_t, 2>{width, height},
             std::array<ptrdiff_t, 2>{sizeof(T) * componentCount_,
                                      vx::checked_cast<ptrdiff_t>(
                                          sizeof(T) * componentCount_ * width)},
             dataSH) {
  // qDebug() << "ImageDataPixel::ImageDataPixel" << this << writableFromDBus;
}
// Put into namespace to avoid clang -Wdtor-name warning, see
// https://github.com/llvm/llvm-project/issues/46323
namespace vx {
template <typename T, size_t componentCount_>
ImageDataPixelInst<T, componentCount_>::~ImageDataPixelInst() {
  // qDebug() << "ImageDataPixel::~ImageDataPixel" << this;
}
}  // namespace vx

template <typename T, size_t componentCount_>
QList<QSharedPointer<vx::SharedMemory>>
vx::ImageDataPixelInst<T, componentCount_>::getSharedMemorySections() {
  return {dataSH};
}

ImageDataPixelOld::ImageDataPixelInst(quint64 width, quint64 height,
                                      bool writableFromDBus,
                                      vx::DataType dataType)
    : ImageDataPixel(dataType, writableFromDBus),
      image_(width, height, true, writableFromDBus) {
  // qDebug() << "ImageDataPixel::ImageDataPixel" << this << writableFromDBus;
}
ImageDataPixelOld::~ImageDataPixelInst() {
  // qDebug() << "ImageDataPixel::~ImageDataPixel" << this;
}

vx::Array2<ImageDataPixelOld::ElementType> ImageDataPixelOld::array() {
  return vx::Array2<ImageDataPixelOld::ElementType>(
      (std::array<float, 1>*)image_.getBuffer().data(),
      std::array<size_t, 2>{image_.getWidth(), image_.getHeight()},
      std::array<ptrdiff_t, 2>{
          sizeof(float),
          vx::checked_cast<ptrdiff_t>(sizeof(float) * image_.getWidth())},
      QSharedPointer<QObject>()  // TODO:?
  );
}

template <typename T, size_t componentCount>
vx::Array3Info vx::ImageDataPixelInst<T, componentCount>::getData(
    bool writable) {
  // qDebug() << "ImageDataPixel::getData" << object << this->writableFromDBus()
  // << writable;
  // TODO: allow writing into all images?

  if (writable && !this->writableFromDBus())
    throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                    "ImageDataPixel not writable");

  Array3Info info;

  this->dataSH->getHandle(writable, info.handle);
  info.offset = 0;

  getDataTypeInfo(this->dataType(), info.dataType, info.dataTypeSize,
                  info.byteorder);

  info.sizeX = array().template size<0>();
  info.sizeY = array().template size<1>();
  info.sizeZ = componentCount();
  info.strideX = array().template strideBytes<0>();
  info.strideY = array().template strideBytes<1>();
  info.strideZ = sizeof(T);

  return info;
}

vx::Array3Info ImageDataPixelOld::getData(bool writable) {
  // qDebug() << "ImageDataPixel::getData" << object << this->writableFromDBus()
  // << writable;
  // TODO: allow writing into all images?

  if (writable && !this->writableFromDBus())
    throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                    "ImageDataPixel not writable");

  Array3Info info;

  this->image().getBuffer().getSharedMemory()->getHandle(writable, info.handle);
  info.offset = 0;

  info.dataType = "float";                // TODO
  info.dataTypeSize = sizeof(float) * 8;  // TODO
  if (info.dataTypeSize == 1) {
    info.byteorder = "none";
  } else {
    if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
      info.byteorder = "big";
    } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
      info.byteorder = "little";
    } else {
      info.byteorder = "unknown";
    }
  }

  info.sizeX = width();
  info.sizeY = height();
  info.sizeZ = 1;
  info.strideX = sizeof(float);
  info.strideY = info.strideX * info.sizeX;
  info.strideZ = sizeof(float);

  return info;
}

QList<QSharedPointer<vx::SharedMemory>>
ImageDataPixelOld::getSharedMemorySections() {
  return {image().getSharedMemory()};
}

// This is a 'fake' TomographyRawData2DRegular which is simply a wrapper around
// a single-component image
namespace vx {
template <typename T>
class TomographyRawData2DRegularImpl
    : public vx::TomographyRawData2DRegularInst<T> {
  // TODO: This probably should be a strong pointer but to avoid cycles it then
  // would have to share ownership with the ImageDataPixel shared pointer
  QWeakPointer<ImageDataPixelInst<T, 1>> image;

  friend class TomographyRawData2DRegularAdaptorImpl;

 public:
  TomographyRawData2DRegularImpl(const QSharedPointer<ImageDataPixel>& image);
  ~TomographyRawData2DRegularImpl() {}

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override {
    auto img = image.lock();
    if (!img)
      return {};
    else
      return img->getSharedMemorySections();
  }

  vx::TupleVector<double, 2> gridSpacing() override {
    // TODO: Not implemented
    return vx::TupleVector<double, 2>(1, 1);
  }

  quint64 imageCount() override {
    return 1;  // Always 1 image
  }

  vx::TupleVector<double, 2> imageOrigin() override {  // TODO: Not implemented
    return vx::TupleVector<double, 2>(0, 0);
  }

  vx::TupleVector<quint64, 2> imageShape() override {
    auto img = image.lock();
    if (!img)
      // This should return an error, seems not to be supported by QDBus
      return std::make_tuple(0, 0);
    return std::make_tuple(img->width(), img->height());
  }

  vx::Array3Info getDataReadonly() override {
    auto img = image.lock();
    if (!img)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Image has already been destroyed");

    return img->getData(false);
  }

  vx::Array3Info getDataWritable(
      const QSharedPointer<vx::DataUpdate>& updateObj) override {
    // TODO: Currently updates / version for ImageDataPixel and
    // TomographyRawData2DRegularImpl are distinct from each other

    auto img = image.lock();
    if (!img)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Image has already been destroyed");

    updateObj->validateCanUpdate(this);

    return img->getData(true);
  }

  vx::Array3<T> array() override {
    auto img = image.lock();
    if (!img)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Image has already been destroyed");

    const auto& array2 = img->array();

    std::array<size_t, 3> shape{array2.template size<0>(),
                                array2.template size<1>(), 1};
    std::array<ptrdiff_t, 3> stride{array2.template strideBytes<0>(),
                                    array2.template strideBytes<1>(), 0};

    // This cast hopefully should be ok
    std::array<T, 1>* ptr2 = array2.data();
    auto ptr3 = reinterpret_cast<T*>(ptr2);

    vx::Array3<T> array3(ptr3, shape, stride, array2.getBackend());

    return array3;
  }
};
}  // namespace vx
template <typename T>
vx::TomographyRawData2DRegularImpl<T>::TomographyRawData2DRegularImpl(
    const QSharedPointer<ImageDataPixel>& image)
    : TomographyRawData2DRegularInst<T>(image->dataType()),
      image(qSharedPointerDynamicCast<ImageDataPixelInst<T, 1>>(image)) {
  if (image->componentCount() != 1)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "image->componentCount() != 1");
  if (!image)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError", "Cast failed");
}

template <typename T, size_t componentCount>
QSharedPointer<vx::TomographyRawData2DRegular>
vx::ImageDataPixelInst<T, componentCount>::fakeTomographyRawData2DRegular() {
  QMutexLocker locker(&fakeTomographyRawData2DRegularMutex);

  if (fakeTomographyRawData2DRegular_) return fakeTomographyRawData2DRegular_;
  fakeTomographyRawData2DRegular_ =
      createBase<TomographyRawData2DRegularImpl<T>>(thisShared());
  return fakeTomographyRawData2DRegular_;
}

QSharedPointer<vx::TomographyRawData2DRegular>
ImageDataPixelOld::fakeTomographyRawData2DRegular() {
  QMutexLocker locker(&fakeTomographyRawData2DRegularMutex);

  if (fakeTomographyRawData2DRegular_) return fakeTomographyRawData2DRegular_;
  fakeTomographyRawData2DRegular_ =
      createBase<TomographyRawData2DRegularImpl<float>>(thisShared());
  return fakeTomographyRawData2DRegular_;
}

namespace vx {

#define INST_T(T)                          \
  template class ImageDataPixelInst<T, 1>; \
  template class ImageDataPixelInst<T, 3>; \
  template class ImageDataPixelInst<T, 4>;

#define INST_T_NO1(T)                      \
  template class ImageDataPixelInst<T, 3>; \
  template class ImageDataPixelInst<T, 4>;

INST_T_NO1(float)
// INST_T(double)
// INST_T(int8_t)
// INST_T(int16_t)
// INST_T(int32_t)
// INST_T(int64_t)
INST_T(uint8_t)
// INST_T(uint16_t)
// INST_T(uint32_t)
// INST_T(uint64_t)

}  // namespace vx
