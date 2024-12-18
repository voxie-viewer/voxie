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

#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieClient/ArrayInfo.hpp>
#include <VoxieClient/Exception.hpp>

namespace vx {
class TomographyRawData2DRegular;

template <typename T, size_t componentCount>
class VOXIEBACKEND_EXPORT ImageDataPixelInst;

class VOXIEBACKEND_EXPORT ImageDataPixel : public vx::Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  DataType dataType_;

  template <typename T>
  static inline QSharedPointer<ImageDataPixel> createInst2(
      size_t width, size_t height, size_t componentCount, DataType dataType,
      bool writableFromDBus);

 protected:
  bool writableFromDBus_;

  // TODO: This probably should share ownership with the ImageDataPixel
  QSharedPointer<TomographyRawData2DRegular> fakeTomographyRawData2DRegular_ =
      QSharedPointer<TomographyRawData2DRegular>();
  QMutex fakeTomographyRawData2DRegularMutex;

  // throws Exception
  ImageDataPixel(DataType dataType, bool writableFromDBus);

 public:
  static QSharedPointer<ImageDataPixel> createInst(size_t width, size_t height,
                                                   size_t componentCount,
                                                   DataType dataType,
                                                   bool writableFromDBus);

  ~ImageDataPixel() override;

  QList<QString> supportedDBusInterfaces() override;

  virtual vx::Array3Info getData(bool writable) = 0;

  bool writableFromDBus() const { return writableFromDBus_; }

  DataType dataType() const { return dataType_; }

  virtual quint64 componentCount() = 0;

  virtual quint64 width() = 0;
  virtual quint64 height() = 0;

  template <typename F,
            typename Ret = decltype((*(F*)nullptr)(
                (*(QSharedPointer<ImageDataPixelInst<float, 4>>*)nullptr)))>
  inline Ret performInGenericContext(const F& lambda);

  template <size_t componentCount_, typename F,
            typename Ret = decltype((*(F*)nullptr)(
                (*(QSharedPointer<
                    ImageDataPixelInst<float, componentCount_>>*)nullptr)))>
  inline Ret performInGenericContextWithComponents(const F& lambda);

  // Currently only works for 4-component float images (with values between 0
  // and 1)
  QImage convertToQImage();

  // Currently only works for 4-component float images (values will be between 0
  // and 1) and QImages with a format of QImage::Format_ARGB32
  void fromQImage(const QImage& image,
                  const vx::VectorSizeT2& outputRegionStart);

  virtual QSharedPointer<TomographyRawData2DRegular>
  fakeTomographyRawData2DRegular() = 0;

 private:
  template <size_t componentCount, typename T, typename F, typename Ret>
  inline Ret performInGenericContextImpl(const F& lambda);
};

}  // namespace vx
