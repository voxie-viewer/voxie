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

#include "TomographyRawData2DRegularShmemInst.hpp"

#include <VoxieBackend/Data/SharedMemory.hpp>

using namespace vx;

template <typename T>
TomographyRawData2DRegularShmemInst<T>::TomographyRawData2DRegularShmemInst(
    quint64 width, quint64 height, quint64 numberOfImages, DataType dataType)
    : TomographyRawData2DRegularInst<T>(dataType),
      dataSH(
          createQSharedPointer<SharedMemory>(checked_cast<size_t>(checked_mul(
              width, checked_mul(height, checked_mul<quint64>(numberOfImages,
                                                              sizeof(T))))))),
      array_(
          (T*)dataSH->getData(),
          std::array<size_t, 3>{width, height, numberOfImages},
          std::array<ptrdiff_t, 3>{
              sizeof(T),
              vx::checked_cast<ptrdiff_t>(sizeof(T) * width),
              vx::checked_cast<ptrdiff_t>(sizeof(T) * width * numberOfImages),
          },
          dataSH) {}
template <typename T>
TomographyRawData2DRegularShmemInst<T>::~TomographyRawData2DRegularShmemInst() {
}

template <typename T>
vx::TupleVector<double, 2>
TomographyRawData2DRegularShmemInst<T>::gridSpacing() {
  // TODO: Remove this?
  return vx::TupleVector<double, 2>{1, 1};
}

template <typename T>
quint64 TomographyRawData2DRegularShmemInst<T>::imageCount() {
  return this->array_.template size<2>();
}

template <typename T>
vx::TupleVector<double, 2>
TomographyRawData2DRegularShmemInst<T>::imageOrigin() {
  // TODO: Remove this?
  return vx::TupleVector<double, 2>{0, 0};
}

template <typename T>
vx::TupleVector<quint64, 2>
TomographyRawData2DRegularShmemInst<T>::imageShape() {
  return vx::TupleVector<quint64, 2>{this->array_.template size<0>(),
                                     this->array_.template size<1>()};
}

template <typename T>
vx::Array3Info TomographyRawData2DRegularShmemInst<T>::getDataReadonly() {
  Array3Info info;

  this->dataSH->getHandle(false, info.handle);
  info.offset = 0;

  getDataTypeInfo(this->dataType(), info.dataType, info.dataTypeSize,
                  info.byteorder);

  info.sizeX = array_.template size<0>();
  info.sizeY = array_.template size<1>();
  info.sizeZ = array_.template size<2>();
  info.strideX = array_.template strideBytes<0>();
  info.strideY = array_.template strideBytes<1>();
  info.strideZ = array_.template strideBytes<2>();

  return info;
}

template <typename T>
vx::Array3Info TomographyRawData2DRegularShmemInst<T>::getDataWritable(
    const QSharedPointer<vx::DataUpdate>& update) {
  update->validateCanUpdate(this);

  Array3Info info;

  this->dataSH->getHandle(true, info.handle);
  info.offset = 0;

  getDataTypeInfo(this->dataType(), info.dataType, info.dataTypeSize,
                  info.byteorder);

  info.sizeX = array_.template size<0>();
  info.sizeY = array_.template size<1>();
  info.sizeZ = array_.template size<2>();
  info.strideX = array_.template strideBytes<0>();
  info.strideY = array_.template strideBytes<1>();
  info.strideZ = array_.template strideBytes<2>();

  return info;
}

template <typename T>
vx::Array3<T> TomographyRawData2DRegularShmemInst<T>::array() {
  return array_;
}

template <typename T>
QList<QSharedPointer<SharedMemory>>
TomographyRawData2DRegularShmemInst<T>::getSharedMemorySections() {
  return {this->dataSH};
}

#define INST_T(T) template class vx::TomographyRawData2DRegularShmemInst<T>;

INST_T(float)
// INST_T(double)
// INST_T(int8_t)
// INST_T(int16_t)
// INST_T(int32_t)
// INST_T(int64_t)
INST_T(uint8_t)
// INST_T(uint16_t)
// INST_T(uint32_t)
// INST_T(uint64_t)
