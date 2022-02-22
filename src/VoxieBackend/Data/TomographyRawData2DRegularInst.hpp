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

#include <VoxieBackend/Data/TomographyRawData2DRegular.hpp>

#include <VoxieClient/Array.hpp>

namespace vx {
template <typename T>
class VOXIEBACKEND_EXPORT TomographyRawData2DRegularInst
    : public vx::TomographyRawData2DRegular {
 public:
  typedef T ValueType;

  TomographyRawData2DRegularInst(DataType dataType);
  ~TomographyRawData2DRegularInst() override;

  virtual vx::Array3<ValueType> array() = 0;
};

template <typename F, typename Ret>
Ret TomographyRawData2DRegular::performInGenericContext(const F& lambda) {
  switch (this->dataType()) {
    // TODO: which data types should be allowed?
    case DataType::Float32:
      return performInGenericContextImpl<float, F, Ret>(lambda);
      /*
    case DataType::Float64:
      return performInGenericContextImpl<double, F,
    Ret>(lambda); case DataType::Int64: return
    performInGenericContextImpl<int64_t, F, Ret>(lambda); case
    DataType::Int32: return performInGenericContextImpl<
    int32_t, F, Ret>(lambda); case DataType::Int16: return
    performInGenericContextImpl<int16_t, F, Ret>(lambda); case
    DataType::Int8: return performInGenericContextImpl<
    int8_t, F, Ret>(lambda);
      */
    case DataType::UInt8:
      return performInGenericContextImpl<uint8_t, F, Ret>(lambda);
      /*
    case DataType::UInt16:
      return performInGenericContextImpl<uint16_t, F,
    Ret>(lambda); case DataType::UInt32: return
    performInGenericContextImpl<uint32_t, F, Ret>(lambda); case
    DataType::UInt64: return performInGenericContextImpl<
    uint64_t, F, Ret>(lambda);
      */
    default:
      throw Exception("de.uni_stuttgart.Voxie.InternalError",
                      "Unknown data type for TomographyRawData2DRegular");
  }
}

template <typename T, typename F, typename Ret>
Ret TomographyRawData2DRegular::performInGenericContextImpl(const F& lambda) {
  auto cast = qSharedPointerDynamicCast<TomographyRawData2DRegularInst<T>>(
      thisShared());
  if (!cast)
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "Cannot cast TomographyRawData2DRegular to "
                    "TomographyRawData2DRegularInst<>");
  return lambda(cast);
}
}  // namespace vx
