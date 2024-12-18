/*
 * Copyright (c) 2010-2012 Steffen Kieß
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

#ifndef HDF5_ENUMTYPE_HPP_INCLUDED
#define HDF5_ENUMTYPE_HPP_INCLUDED

// Enum data type

#include <HDF5/Forward.hpp>

#include <Core/Assert.hpp>
#include <Core/Util.hpp>

#include <hdf5.h>

#include <HDF5/DataType.hpp>

namespace HDF5 {
class IntegerType;

class EnumType : public DataType {
  void checkType() const;

 public:
  EnumType() {}

  explicit EnumType(const IdComponent& value) : DataType(value) {
    checkType();
  }

  // This constructor takes ownership of the object refered to by value
  explicit EnumType(hid_t value) : DataType(value) { checkType(); }

  static EnumType create(const IntegerType& baseType);

  void insert(const std::string& name, const void* value) const;

  size_t nMembers() const;
  std::string memberName(size_t i) const;
  void memberValue(size_t i, void* valueOut) const;
};
}  // namespace HDF5

#endif  // !HDF5_ENUMTYPE_HPP_INCLUDED
