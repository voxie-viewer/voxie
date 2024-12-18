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

#include <VoxieBackend/VoxieBackend.hpp>

#include <QtCore/QMap>

namespace half_float {
// Defined in <half.hpp>
class half;
}  // namespace half_float

namespace vx {

// Defined in <VoxieClient/Bool8.hpp>
class Bool8;

enum class DataType {
  Float16,
  Float32,
  Float64,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  Bool8,
};

#if 0  // DataTypeTraits uses enum parameter
template <DataType dt>
struct DataTypeTraits;
template <typename Type>
struct DataTypeTraitsByTypeImpl;
template <typename Type>
using DataTypeTraitsByType = typename DataTypeTraitsByTypeImpl<Type>::Traits;
template <DataType dt>
using DataTypeTraitsByEnum = DataTypeTraits<dt>;
// TODO: DataTypeTraits<...>::dataType needs a definition before C++17
#define DECL(en, ty)                                           \
  template <>                                                  \
  struct DataTypeTraits<vx::DataType::en> {                    \
    static constexpr vx::DataType dataType = DataType::en;     \
    static vx::DataType getDataType() { return DataType::en; } \
    using Type = ty;                                           \
  };                                                           \
  template <>                                                  \
  struct DataTypeTraitsByTypeImpl<ty> {                        \
    using Traits = DataTypeTraits<vx::DataType::en>;           \
  };
#else  // DataTypeTraits uses type parameter
template <typename Type>
struct DataTypeTraits;
template <DataType dt>
struct DataTypeTraitsByEnumImpl;
template <typename Type>
using DataTypeTraitsByType = DataTypeTraits<Type>;
template <DataType dt>
using DataTypeTraitsByEnum = typename DataTypeTraitsByEnumImpl<dt>::Traits;
// TODO: DataTypeTraits<...>::dataType needs a definition before C++17
#define DECL(en, ty)                                           \
  template <>                                                  \
  struct DataTypeTraits<ty> {                                  \
    static constexpr vx::DataType dataType = DataType::en;     \
    static vx::DataType getDataType() { return DataType::en; } \
    using Type = ty;                                           \
  };                                                           \
  template <>                                                  \
  struct DataTypeTraitsByEnumImpl<vx::DataType::en> {          \
    using Traits = DataTypeTraits<ty>;                         \
  };
#endif

// TODO: provide another type (float) for doing calculations for half?
DECL(Float16, half_float::half)
DECL(Float32, float)
DECL(Float64, double)
DECL(Int8, std::int8_t)
DECL(Int16, std::int16_t)
DECL(Int32, std::int32_t)
DECL(Int64, std::int64_t)
DECL(UInt8, std::uint8_t)
DECL(UInt16, std::uint16_t)
DECL(UInt32, std::uint32_t)
DECL(UInt64, std::uint64_t)
DECL(Bool8, vx::Bool8)
#undef DECL

template <vx::DataType val>
struct DataTypeListHelper {
  static constexpr vx::DataType value = val;
};

template <vx::DataType... values>
struct DataTypeList {
  template <size_t N>
  static constexpr vx::DataType element = std::tuple_element<
      N, std::tuple<DataTypeListHelper<values>...>>::type::value;

  // Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67248 /
  // https://bugs.llvm.org/show_bug.cgi?id=24473 see
  // https://stackoverflow.com/questions/32050119/variable-template-in-template-class-unexpected-error-possible-bug
  template <size_t N>
  using ElementHelper = typename std::tuple_element<
      N, std::tuple<DataTypeListHelper<values>...>>::type;

  static constexpr size_t count = sizeof...(values);
};

VOXIEBACKEND_EXPORT Q_NORETURN void switchOverDataTypeFailed(DataType dt);
template <typename List, typename RetType, typename F, size_t pos = 0,
          std::enable_if_t<pos == List::count, bool> = true>
RetType switchOverDataType(vx::DataType dt, const F& fun) {
  Q_UNUSED(fun);
  switchOverDataTypeFailed(dt);
}
template <typename List, typename RetType, typename F, size_t pos = 0,
          std::enable_if_t<pos<List::count, bool> = true> RetType
              switchOverDataType(vx::DataType dt, const F& fun) {
  // if (dt == List::template element<pos>)
  //  return fun(DataTypeTraitsByEnum<List::template element<pos>>());
  if (dt == List::template ElementHelper<pos>::value)
    return fun(
        DataTypeTraitsByEnum<List::template ElementHelper<pos>::value>());
  return switchOverDataType<List, RetType, F, pos + 1>(dt, fun);
}

// TODO: Remove / make intern
VOXIEBACKEND_EXPORT extern QMap<DataType, QString> DataTypeNames;

VOXIEBACKEND_EXPORT void getDataTypeInfo(DataType type, QString& dataType,
                                         quint32& dataTypeSize,
                                         QString& byteorder,
                                         bool getAsNative = false);

VOXIEBACKEND_EXPORT DataType parseDataTypeString(const QString& type);
VOXIEBACKEND_EXPORT QString getDataTypeString(DataType type);

VOXIEBACKEND_EXPORT QString getNativeByteorder();

VOXIEBACKEND_EXPORT DataType
parseDataTypeStruct(const std::tuple<QString, quint32, QString>& type);
VOXIEBACKEND_EXPORT std::tuple<QString, quint32, QString> getDataTypeStruct(
    DataType type, bool getAsNative = false);

size_t getElementSizeBytes(DataType type);
}  // namespace vx
