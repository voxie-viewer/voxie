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

#include "ArrayInfo.hpp"

vx::Array1Info::Array1Info(const vx::Array1InfoDBus& info) {
  handle = std::get<0>(info);
  offset = std::get<1>(info);
  dataType = std::get<0>(std::get<2>(info));
  dataTypeSize = std::get<1>(std::get<2>(info));
  byteorder = std::get<2>(std::get<2>(info));
  size = std::get<0>(std::get<3>(info));
  stride = std::get<0>(std::get<4>(info));
  metadata = std::get<5>(info);
}

vx::Array1InfoDBus vx::Array1Info::toDBus() const {
  return std::make_tuple(
      handle, offset, std::make_tuple(dataType, dataTypeSize, byteorder),
      std::make_tuple(size), std::make_tuple(stride), metadata);
}

vx::Array2Info::Array2Info(const vx::Array2InfoDBus& info) {
  handle = std::get<0>(info);
  offset = std::get<1>(info);
  dataType = std::get<0>(std::get<2>(info));
  dataTypeSize = std::get<1>(std::get<2>(info));
  byteorder = std::get<2>(std::get<2>(info));
  sizeX = std::get<0>(std::get<3>(info));
  sizeY = std::get<1>(std::get<3>(info));
  strideX = std::get<0>(std::get<4>(info));
  strideY = std::get<1>(std::get<4>(info));
  metadata = std::get<5>(info);
}

vx::Array2InfoDBus vx::Array2Info::toDBus() const {
  return std::make_tuple(handle, offset,
                         std::make_tuple(dataType, dataTypeSize, byteorder),
                         std::make_tuple(sizeX, sizeY),
                         std::make_tuple(strideX, strideY), metadata);
}

vx::Array3Info::Array3Info(const vx::Array3InfoDBus& info) {
  handle = std::get<0>(info);
  offset = std::get<1>(info);
  dataType = std::get<0>(std::get<2>(info));
  dataTypeSize = std::get<1>(std::get<2>(info));
  byteorder = std::get<2>(std::get<2>(info));
  sizeX = std::get<0>(std::get<3>(info));
  sizeY = std::get<1>(std::get<3>(info));
  sizeZ = std::get<2>(std::get<3>(info));
  strideX = std::get<0>(std::get<4>(info));
  strideY = std::get<1>(std::get<4>(info));
  strideZ = std::get<2>(std::get<4>(info));
  metadata = std::get<5>(info);
}

vx::Array3InfoDBus vx::Array3Info::toDBus() const {
  return std::make_tuple(handle, offset,
                         std::make_tuple(dataType, dataTypeSize, byteorder),
                         std::make_tuple(sizeX, sizeY, sizeZ),
                         std::make_tuple(strideX, strideY, strideZ), metadata);
}
