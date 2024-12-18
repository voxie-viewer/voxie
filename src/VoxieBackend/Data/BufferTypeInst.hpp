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

#include <VoxieBackend/Data/BufferType.hpp>
#include <VoxieBackend/Data/DataType.hpp>

#include <VoxieClient/Vector.hpp>

namespace vx {
template <typename T>
inline QSharedPointer<vx::BufferTypeBase> createBufferTypeBase();

template <typename T>
struct BufferTypeTraits : T {
  static QSharedPointer<vx::BufferTypeBase> createType() {
    return createQSharedPointer<BufferTypeStruct>(T::getMembers());
  }
};

template <typename T>
struct BufferTypeTraitsPrimitive {
  static QSharedPointer<vx::BufferTypeBase> createType() {
    return createQSharedPointer<BufferTypePrimitive>(
        vx::DataTypeTraitsByType<T>::getDataType());
  }
};

#define VX_BUFFER_TYPE_PRIMITIVE(T) \
  template <>                       \
  struct BufferTypeTraits<T> : BufferTypeTraitsPrimitive<T> {};
VX_BUFFER_TYPE_PRIMITIVE(std::uint8_t)
VX_BUFFER_TYPE_PRIMITIVE(std::int8_t)
VX_BUFFER_TYPE_PRIMITIVE(std::uint16_t)
VX_BUFFER_TYPE_PRIMITIVE(std::int16_t)
VX_BUFFER_TYPE_PRIMITIVE(std::uint32_t)
VX_BUFFER_TYPE_PRIMITIVE(std::int32_t)
VX_BUFFER_TYPE_PRIMITIVE(std::uint64_t)
VX_BUFFER_TYPE_PRIMITIVE(std::int64_t)
VX_BUFFER_TYPE_PRIMITIVE(half_float::half)
VX_BUFFER_TYPE_PRIMITIVE(float)
VX_BUFFER_TYPE_PRIMITIVE(double)
VX_BUFFER_TYPE_PRIMITIVE(bool)

template <typename T, size_t dim>
struct BufferTypeTraits<vx::Vector<T, dim>> {
  static QSharedPointer<vx::BufferTypeBase> createType() {
    QList<quint64> shape{dim};
    QList<qint64> stridesBytes{sizeof(T)};

    return createQSharedPointer<BufferTypeArray>(shape, stridesBytes,
                                                 createBufferTypeBase<T>());
  }
};

template <typename T>
inline QSharedPointer<vx::BufferTypeBase> createBufferTypeBase() {
  return BufferTypeTraits<T>::createType();
}

template <typename T, typename V>
BufferTypeStruct::Member createBufferTypeMemberInfo(
    const vx::Optional<QString>& name, V T::*member) {
  static_assert(std::is_standard_layout<T>::value,
                "T is not a standard layout type");

  // This works, but is not portable
  // size_t offset =  ((char*) &(((T*) 0)->*ptr)) - ((char*) 0);

  // This should be more or less portable. The compiler should optimize this
  // to the above form
  typename std::aligned_storage<sizeof(V), std::alignment_of<V>::value>::type
      val;
  size_t offset = ((char*)&(((T*)&val)->*member)) - ((char*)&val);

  return BufferTypeStruct::Member(name, offset, createBufferTypeBase<V>());
}
#define VX_BUFFER_TYPE_MEMBER(name) \
  createBufferTypeMemberInfo(vx::Optional<QString>(#name), &Type::name)
#define VX_BUFFER_TYPE_MEMBER_ANON(name) \
  createBufferTypeMemberInfo(vx::nullopt, &Type::name)

template <typename T>
inline QSharedPointer<vx::BufferType> createBufferType() {
  using Type = typename T::Type;

  return makeSharedQObject<BufferType>(
      T::getName(), createBufferTypeBase<Type>(), sizeof(Type));
}

#define VX_BUFFER_TYPE_DEFINE(name, strname)                           \
  const char* name::getName() { return strname; }                      \
  QSharedPointer<vx::BufferType> name::bufferType() {                  \
    static QSharedPointer<BufferType> type = createBufferType<name>(); \
    return type;                                                       \
  }

}  // namespace vx
