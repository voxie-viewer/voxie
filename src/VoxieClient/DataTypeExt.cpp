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

#include "DataTypeExt.hpp"

#include <VoxieClient/Exception.hpp>

#include <tuple>

QString vx::baseTypeToString(BaseType baseType) {
  switch (baseType) {
#define DECL(t, s)  \
  case BaseType::t: \
    return s;
    DECL(Float, "float")
    DECL(Int, "int")
    DECL(UInt, "uint")
    DECL(Bool, "bool")
#undef DECL
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Invalid base type");
  }
}
QString vx::endiannessToString(Endianness endianness) {
  switch (endianness) {
#define DECL(t, s)    \
  case Endianness::t: \
    return s;
    DECL(Little, "little")
    DECL(Big, "big")
    // Note: No Native here, either Little or Big should match
#undef DECL
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Invalid endianness");
  }
}
vx::BaseType vx::parseBaseType(const QString& str) {
#define DECL(t, s) \
  if (str == s) return BaseType::t;
  DECL(Float, "float")
  DECL(Int, "int")
  DECL(UInt, "uint")
  DECL(Bool, "bool")
#undef DECL
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "Unknown base type");
}
vx::Endianness vx::parseEndianness(const QString& str) {
#define DECL(t, s) \
  if (str == s) return Endianness::t;
  DECL(Little, "little")
  DECL(Big, "big")
  DECL(Native, "native")
#undef DECL
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "Unknown endianness");
}

QString vx::DataTypeExt::toString() {
  return "(" + baseTypeToString(baseType()) + ", " +
         QString::number(numberOfBits()) + ", " +
         endiannessToString(endianness()) + ")";
}

vx::DataTypeExt vx::parseDataTypeExt(
    const std::tuple<QString, unsigned int, QString>& value) {
  return DataTypeExt(parseBaseType(std::get<0>(value)), std::get<1>(value),
                     parseEndianness(std::get<2>(value)));
}

std::tuple<QString, quint32, QString> vx::DataTypeExt::toTuple() {
  return std::make_tuple(baseTypeToString(baseType()), numberOfBits(),
                         endiannessToString(endianness()));
}

Q_NORETURN void vx::switchOverDataTypeExtFailed(DataTypeExt dt) {
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.Error",
      "Data type unsupported in this context: " + dt.toString());
}
