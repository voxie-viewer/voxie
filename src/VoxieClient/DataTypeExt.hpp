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

#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QString>

namespace half_float {
// Defined in <half.hpp>
class half;
}  // namespace half_float

namespace vx {
// Defined in <VoxieClient/Bool8.hpp>
class Bool8;

enum class BaseType {
  Float,
  Int,
  UInt,
  Bool,
};
enum class Endianness {
  None,  // Only for 8-bit values
  Big,
  Little,
// TODO: Use std::endian (requires C++20)
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  Native = Little,
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  Native = Big,
// Note: This assumes that win32 is always LE, see
// https://en.cppreference.com/w/cpp/types/endian
#elif defined(_WIN32)
  Native = Little,
#else
#error "Cannot determine endianness"
#endif
};

class VOXIECLIENT_EXPORT DataTypeExt {
  BaseType baseType_;
  quint32 numberOfBits_;
  Endianness endianness_;

 public:
  DataTypeExt(const BaseType& baseType, quint32 numberOfBits,
              const Endianness& endianness)
      : baseType_(baseType),
        numberOfBits_(numberOfBits),
        endianness_(endianness) {}

  BaseType baseType() const { return baseType_; }
  quint32 numberOfBits() const { return numberOfBits_; }
  Endianness endianness() const { return endianness_; }

  friend bool operator==(const DataTypeExt& dt1, const DataTypeExt& dt2) {
    return dt1.baseType() == dt2.baseType() &&
           dt1.numberOfBits() == dt2.numberOfBits() &&
           dt1.endianness() == dt2.endianness();
  }

  QString toString();

  std::tuple<QString, quint32, QString> toTuple();
};
VOXIECLIENT_EXPORT QString baseTypeToString(BaseType baseType);
VOXIECLIENT_EXPORT QString endiannessToString(Endianness endianness);
VOXIECLIENT_EXPORT BaseType parseBaseType(const QString& str);
VOXIECLIENT_EXPORT Endianness parseEndianness(const QString& str);
VOXIECLIENT_EXPORT DataTypeExt
parseDataTypeExt(const std::tuple<QString, unsigned int, QString>& value);

template <BaseType baseType_, quint32 numberOfBits_, Endianness endianness_>
class DataTypeMeta {
 public:
  static constexpr BaseType baseType = baseType_;
  static constexpr quint32 numberOfBits = numberOfBits_;
  static constexpr Endianness endianness = endianness_;

  static DataTypeExt dataType() {
    return DataTypeExt(baseType, numberOfBits, endianness);
  }
};
// See
// https://stackoverflow.com/questions/34053606/understanding-static-constexpr-member-variables
template <BaseType baseType_, quint32 numberOfBits_, Endianness endianness_>
constexpr BaseType
    DataTypeMeta<baseType_, numberOfBits_, endianness_>::baseType;
template <BaseType baseType_, quint32 numberOfBits_, Endianness endianness_>
constexpr quint32
    DataTypeMeta<baseType_, numberOfBits_, endianness_>::numberOfBits;
template <BaseType baseType_, quint32 numberOfBits_, Endianness endianness_>
constexpr Endianness
    DataTypeMeta<baseType_, numberOfBits_, endianness_>::endianness;

template <typename Type>
struct DataTypeExtTraits;
template <typename Type>
using DataTypeExtTraitsByType = DataTypeExtTraits<Type>;
template <typename MetaType>
struct DataTypeExtTraitsByMetaTypeImpl;
template <typename MetaType>
using DataTypeExtTraitsByMetaType =
    typename DataTypeExtTraitsByMetaTypeImpl<MetaType>::Traits;
#define DECL(baseType_, numberOfBits_, endianness_, ty)                \
  template <>                                                          \
  struct DataTypeExtTraits<ty> {                                       \
    static constexpr BaseType baseType = BaseType::baseType_;          \
    static constexpr quint32 numberOfBits = numberOfBits_;             \
    static constexpr Endianness endianness = Endianness::endianness_;  \
    static DataTypeExt dataType() {                                    \
      return DataTypeExt(baseType, numberOfBits, endianness);          \
    }                                                                  \
    using MetaType = DataTypeMeta<baseType, numberOfBits, endianness>; \
    using Type = ty;                                                   \
  };                                                                   \
  template <>                                                          \
  struct DataTypeExtTraitsByMetaTypeImpl<DataTypeMeta<                 \
      BaseType::baseType_, numberOfBits_, Endianness::endianness_>> {  \
    using Traits = DataTypeExtTraits<ty>;                              \
  };

// TODO: provide another type (float) for doing calculations for half?
DECL(Float, 16, Native, half_float::half)
DECL(Float, 32, Native, float)
DECL(Float, 64, Native, double)
DECL(Int, 8, None, std::int8_t)
DECL(Int, 16, Native, std::int16_t)
DECL(Int, 32, Native, std::int32_t)
DECL(Int, 64, Native, std::int64_t)
DECL(UInt, 8, None, std::uint8_t)
DECL(UInt, 16, Native, std::uint16_t)
DECL(UInt, 32, Native, std::uint32_t)
DECL(UInt, 64, Native, std::uint64_t)
DECL(Bool, 8, None, vx::Bool8)
#undef DECL

template <typename... MetaTypes>
struct DataTypeExtList {
  using AsTuple = std::tuple<MetaTypes...>;

  template <size_t N>
  using MetaType = typename std::tuple_element<N, AsTuple>::type;

  static constexpr size_t count = sizeof...(MetaTypes);
};
template <typename... MetaTypes>
constexpr size_t DataTypeExtList<MetaTypes...>::count;

struct AllSupportedTypesVolume
    : vx::DataTypeExtList<
          vx::DataTypeMeta<vx::BaseType::Float, 16, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Float, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Float, 64, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Int, 8, vx::Endianness::None>,
          vx::DataTypeMeta<vx::BaseType::Int, 16, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Int, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Int, 64, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 8, vx::Endianness::None>,
          vx::DataTypeMeta<vx::BaseType::UInt, 16, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 64, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::Bool, 8, vx::Endianness::None>> {};

struct AllSupportedTypesRawData
    : vx::DataTypeExtList<
          vx::DataTypeMeta<vx::BaseType::Float, 32, vx::Endianness::Native>,
          vx::DataTypeMeta<vx::BaseType::UInt, 8, vx::Endianness::None>> {};

VOXIECLIENT_EXPORT Q_NORETURN void switchOverDataTypeExtFailed(DataTypeExt dt);
template <typename List, typename RetType, typename F, size_t pos = 0,
          std::enable_if_t<pos == List::count, bool> = true>
RetType switchOverDataTypeExt(vx::DataTypeExt dt, const F& fun) {
  Q_UNUSED(fun);
  switchOverDataTypeExtFailed(dt);
}
template <typename List, typename RetType, typename F, size_t pos = 0,
          std::enable_if_t<pos<List::count, bool> = true> RetType
              switchOverDataTypeExt(vx::DataTypeExt dt, const F& fun) {
  if (dt == List::template MetaType<pos>::dataType())
    return fun(
        DataTypeExtTraitsByMetaType<typename List::template MetaType<pos>>());
  return switchOverDataTypeExt<List, RetType, F, pos + 1>(dt, fun);
}
}  // namespace vx
