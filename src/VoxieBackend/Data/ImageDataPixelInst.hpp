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

#include <VoxieClient/Array.hpp>
#include <VoxieClient/Exception.hpp>

#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

namespace vx {
class SharedMemory;

template <typename T, size_t componentCount_>
class VOXIEBACKEND_EXPORT ImageDataPixelInst : public ImageDataPixel {
 public:
  using ComponentType = T;
  using ElementType = std::array<ComponentType, componentCount_>;

 private:
  QSharedPointer<SharedMemory> dataSH;
  vx::Array2<ElementType> array_;

 public:
  // throws Exception
  ImageDataPixelInst(quint64 width, quint64 height, bool writableFromDBus,
                     DataType dataType);
  ~ImageDataPixelInst() override;

  vx::Array3Info getData(bool writable) override final;

  const vx::Array2<ElementType>& array() { return array_; }

  quint64 componentCount() override final { return componentCount_; }

  quint64 width() override final { return array().template size<0>(); }
  quint64 height() override final { return array().template size<1>(); }

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

  QSharedPointer<TomographyRawData2DRegular> fakeTomographyRawData2DRegular()
      override;
};

// TODO: Merge OpenCL stuff into generic class
template <>
class VOXIEBACKEND_EXPORT ImageDataPixelInst<float, 1> : public ImageDataPixel {
 public:
  using ComponentType = float;
  using ElementType = std::array<ComponentType, 1>;

 private:
  FloatImage image_;

 public:
  // throws Exception
  ImageDataPixelInst(quint64 width, quint64 height, bool writableFromDBus,
                     DataType dataType);
  ~ImageDataPixelInst() override;

  vx::Array3Info getData(bool writable) override;

  vx::Array2<ElementType> array();

  FloatImage& image() { return image_; }

  quint64 componentCount() override { return 1; }

  quint64 width() override { return image().getWidth(); }
  quint64 height() override { return image().getHeight(); }

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

  QSharedPointer<TomographyRawData2DRegular> fakeTomographyRawData2DRegular()
      override;
};

template <size_t componentCount_, typename F, typename Ret>
Ret ImageDataPixel::performInGenericContextWithComponents(const F& lambda) {
  if (this->componentCount() != componentCount_)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "Invalid number of components");

  switch (this->dataType()) {
    // TODO: which data types should be allowed?
    case DataType::Float32:
      return performInGenericContextImpl<componentCount_, float, F, Ret>(
          lambda);
      /*
    case DataType::Float64:
      return performInGenericContextImpl<componentCount_, double, F,
    Ret>(lambda); case DataType::Int64: return
    performInGenericContextImpl<componentCount_, int64_t, F, Ret>(lambda); case
    DataType::Int32: return performInGenericContextImpl<componentCount_,
    int32_t, F, Ret>(lambda); case DataType::Int16: return
    performInGenericContextImpl<componentCount_, int16_t, F, Ret>(lambda); case
    DataType::Int8: return performInGenericContextImpl<componentCount_,
    int8_t, F, Ret>(lambda);
      */
    case DataType::UInt8:
      return performInGenericContextImpl<componentCount_, uint8_t, F, Ret>(
          lambda);
      /*
    case DataType::UInt16:
      return performInGenericContextImpl<componentCount_, uint16_t, F,
    Ret>(lambda); case DataType::UInt32: return
    performInGenericContextImpl<componentCount_, uint32_t, F, Ret>(lambda); case
    DataType::UInt64: return performInGenericContextImpl<componentCount_,
    uint64_t, F, Ret>(lambda);
      */
    default:
      throw Exception("de.uni_stuttgart.Voxie.InternalError",
                      "Unknown data type for ImageDataPixel");
  }
}

template <typename F, typename Ret>
Ret ImageDataPixel::performInGenericContext(const F& lambda) {
  switch (lambda->componentCount()) {
    case 1:
      return performInGenericContextWithComponents<1, F, Ret>(lambda);
    case 3:
      return performInGenericContextWithComponents<3, F, Ret>(lambda);
    case 4:
      return performInGenericContextWithComponents<4, F, Ret>(lambda);
    default:
      throw Exception("de.uni_stuttgart.Voxie.InternalError",
                      "Unknown number of components for ImageDataPixel");
  }
}

template <size_t componentCount_, typename T, typename F, typename Ret>
Ret ImageDataPixel::performInGenericContextImpl(const F& lambda) {
  auto cast = qSharedPointerDynamicCast<ImageDataPixelInst<T, componentCount_>>(
      thisShared());
  if (!cast)
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "Cannot cast ImageDataPixel to ImageDataPixelInst<>");
  return lambda(cast);
}

}  // namespace vx
