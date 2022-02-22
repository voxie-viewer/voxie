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

#include <VoxieClient/ArrayInfo.hpp>
#include <VoxieClient/DBusTypes.hpp>

namespace vx {
template <typename T>
class VOXIEBACKEND_EXPORT TomographyRawData2DRegularInst;

class VOXIEBACKEND_EXPORT TomographyRawData2DRegular : public vx::Data {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(TomographyRawData2DRegular)

  DataType dataType_;

 protected:
  TomographyRawData2DRegular(DataType dataType);
  ~TomographyRawData2DRegular() override;

 public:
  static QSharedPointer<TomographyRawData2DRegular> createInst(
      size_t width, size_t height, size_t numberOfImages, DataType dataType);

  QList<QString> supportedDBusInterfaces() override;

  virtual vx::TupleVector<double, 2> gridSpacing() = 0;

  virtual quint64 imageCount() = 0;

  virtual vx::TupleVector<double, 2> imageOrigin() = 0;

  virtual vx::TupleVector<quint64, 2> imageShape() = 0;

  virtual vx::Array3Info getDataReadonly() = 0;

  virtual vx::Array3Info getDataWritable(
      const QSharedPointer<vx::DataUpdate>& update) = 0;

  DataType dataType() const { return dataType_; }

  template <
      typename F,
      typename Ret = decltype((*(F*)nullptr)(
          (*(QSharedPointer<TomographyRawData2DRegularInst<float>>*)nullptr)))>
  inline Ret performInGenericContext(const F& lambda);

 private:
  template <typename T, typename F, typename Ret>
  inline Ret performInGenericContextImpl(const F& lambda);
};

}  // namespace vx
