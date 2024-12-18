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

#include "BoolHdf5.hpp"

#include <HDF5/EnumType.hpp>

HDF5::DataType HDF5::TypeImpl<vx::Bool8>::createClassH5Type() {
  // Note: h5py seems to use int8_t, but uint8_t probably makes more sense and
  // h5py also accepts it.
  using BaseType = uint8_t;
  BaseType false_value = 0;
  BaseType true_value = 1;

  auto ty = HDF5::EnumType::create((HDF5::IntegerType)getH5Type<BaseType>());
  ty.insert("FALSE", &false_value);
  ty.insert("TRUE", &true_value);

  return std::move(ty);
}
