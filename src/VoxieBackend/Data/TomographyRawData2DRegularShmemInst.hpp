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

#include <VoxieBackend/Data/TomographyRawData2DRegularInst.hpp>

namespace vx {
template <typename T>
class VOXIEBACKEND_EXPORT TomographyRawData2DRegularShmemInst
    : public vx::TomographyRawData2DRegularInst<T> {
 private:
  QSharedPointer<SharedMemory> dataSH;
  vx::Array3<T> array_;

 public:
  TomographyRawData2DRegularShmemInst(quint64 width, quint64 height,
                                      quint64 numberOfImages,
                                      DataType dataType);
  ~TomographyRawData2DRegularShmemInst() override;

  vx::TupleVector<double, 2> gridSpacing() override;

  quint64 imageCount() override;

  vx::TupleVector<double, 2> imageOrigin() override;

  vx::TupleVector<quint64, 2> imageShape() override;

  vx::Array3Info getDataReadonly() override;

  vx::Array3Info getDataWritable(
      const QSharedPointer<vx::DataUpdate>& update) override;

  vx::Array3<T> array() override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};
}  // namespace vx
