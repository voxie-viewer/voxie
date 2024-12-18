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

#include "EnumType.hpp"

#include <HDF5/DataTypes.hpp>
#include <HDF5/Group.hpp>

#include <Core/Memory.hpp>

namespace HDF5 {
void EnumType::checkType() const {
  if (!isValid()) return;
  switch (getClass()) {
    case H5T_ENUM:
      break;

    default:
      ABORT_MSG("Not an enum datatype");
  }
}

EnumType EnumType::create(const IntegerType& baseType) {
  return EnumType(
      Exception::check("H5Tenum_create", H5Tenum_create(baseType.handle())));
}

void EnumType::insert(const std::string& name, const void* value) const {
  Exception::check("H5Tenum_insert",
                   H5Tenum_insert(handle(), name.c_str(), value));
}

// Note: This is the same as for CompoundType
size_t EnumType::nMembers() const {
  return Exception::check("H5Tget_nmembers", H5Tget_nmembers(handle()));
}

// Note: This is the same as for CompoundType
std::string EnumType::memberName(size_t i) const {
  ASSERT(i <= std::numeric_limits<unsigned>::max());
  unsigned i2 = static_cast<unsigned>(i);
  Core::MallocRefHolder<char> result(HDF5::Exception::check(
      "H5Tget_member_name", H5Tget_member_name(handle(), i2)));
  return result.p;
}

void EnumType::memberValue(size_t i, void* valueOut) const {
  ASSERT(i <= std::numeric_limits<unsigned>::max());
  unsigned i2 = static_cast<unsigned>(i);
  HDF5::Exception::check("H5Tget_member_value",
                         H5Tget_member_value(handle(), i2, valueOut));
}
}  // namespace HDF5
