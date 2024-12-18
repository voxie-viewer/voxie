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

#include <VoxieClient/Bool8.hpp>

#include <HDF5/Matlab.hpp>
#include <HDF5/Type.hpp>

namespace HDF5 {
template <>
class TypeImpl<vx::Bool8> {
 public:
  static HDF5::DataType createClassH5Type();
};

template <>
struct MatlabTypeImpl<vx::Bool8> {
  static HDF5::DataType getMemory() { return getH5Type<vx::Bool8>(); }
  static HDF5::DataType getFile() { return getH5Type<vx::Bool8>(); }
  // TODO: Does this work?
  static std::string matlabClass() { return "logical"; }
  // TODO: Add "MATLAB_int_decode" attribute with value 1 to HDF5 file?
};
}  // namespace HDF5
