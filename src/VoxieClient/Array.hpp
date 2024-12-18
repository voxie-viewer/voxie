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

#include <VoxieClient/ArrayInfo.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/MappedBuffer.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QSharedPointer>

#include <array>

template <typename T>
class ArrayTypeInfo;

// TODO: Use DataTypeExt instead of duplicating things here

// TODO: Merge Array1, Array2 and Array3?

namespace half_float {
// Defined in <half.hpp>
class half;
}  // namespace half_float

namespace vx {
// Defined in VoxieClient/Bool8.hpp
class Bool8;
}  // namespace vx

#define T(tname, nm)                          \
  template <>                                 \
  class ArrayTypeInfo<tname> {                \
   public:                                    \
    static const char* name() { return #nm; } \
  };
T(int8_t, int)
T(int16_t, int)
T(int32_t, int)
T(int64_t, int)
T(uint8_t, uint)
T(uint16_t, uint)
T(uint32_t, uint)
T(uint64_t, uint)
T(half_float::half, float)
T(float, float)
T(double, float)
T(vx::Bool8, bool)
#undef T

// TODO: This is copied from extern/util/C++/Core/CheckedCast.hpp and modified
// TODO: Check whether all this really works
namespace vx {
namespace Intern {
template <typename To, typename From>
Q_NORETURN static void overflow(From value) {
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.Error",
      QString("Overflow while attempting to cast %1").arg(value));
}

template <typename T, typename U>
struct ConversionInfo {
  static const bool sourceInt = std::numeric_limits<U>::is_integer;
  static const bool sourceSigned = std::numeric_limits<U>::is_signed;
  static const U sourceMin = std::numeric_limits<U>::min();
  static const U sourceMax = std::numeric_limits<U>::max();
  static const int sourceDigits = std::numeric_limits<U>::digits;

  static const bool targetSigned = std::numeric_limits<T>::is_signed;
  static const T targetMin = std::numeric_limits<T>::min();
  static const T targetMax = std::numeric_limits<T>::max();
  static const int targetDigits = std::numeric_limits<T>::digits;

  static const bool signedEqual =
      (sourceSigned && targetSigned) || (!sourceSigned && !targetSigned);

  static const bool isWidening =
      sourceInt &&
      (
          // (signedEqual && (targetMin <= sourceMin) && (targetMax >=
          // sourceMax) && ((targetMin < sourceMin) || (targetMax > sourceMax)))
          (signedEqual && targetDigits > sourceDigits) ||
          (targetSigned && !sourceSigned &&
           (targetDigits >= sourceDigits + 1)));

  static const bool isWideningOrEqual =
      sourceInt &&
      (
          // (signedEqual && (targetMin <= sourceMin) && (targetMax >=
          // sourceMax) && ((targetMin < sourceMin) || (targetMax > sourceMax)))
          (signedEqual && targetDigits >= sourceDigits) ||
          (targetSigned && !sourceSigned &&
           (targetDigits >= sourceDigits + 1)));
};

template <typename T, typename U>
struct ConverterU {
  static inline T convert(U v) {
    typedef std::numeric_limits<T> target;

    // unsigned, no need to check for target::min ()
    if (v > target::max()) {
      overflow<T, U>(v);
    }
    return (T)v;
  }
};

template <typename T, typename U>
struct ConverterS {
  static inline T convert(U v) {
    typedef std::numeric_limits<T> target;

    if (v < target::min() || v > target::max()) {
      overflow<T, U>(v);
    }
    return (T)v;
  }
};

template <typename T, typename U>
struct ConverterSU {
  static inline T convert(U v) {
    typedef std::numeric_limits<T> target;

    if (v < 0 || (typename std::make_unsigned<U>::type)v > target::max()) {
      overflow<T, U>(v);
    }

    return (T)v;
  }
};

template <typename T, typename U>
struct ConverterUS {
  static inline T convert(U v) {
    typedef std::numeric_limits<T> target;

    if (v > (typename std::make_unsigned<T>::type)target::max()) {
      overflow<T, U>(v);
    }

    return (T)v;
  }
};

// checked_cast int => cint
template <typename To, typename From>
struct CheckedConverter {
  static_assert(std::numeric_limits<To>::is_integer,
                "std::numeric_limits<To>::is_integer");
  static_assert(std::numeric_limits<From>::is_integer,
                "std::numeric_limits<From>::is_integer");

  typedef ConversionInfo<To, From> Info;
  typedef typename std::conditional<
      Info::signedEqual,
      typename std::conditional<Info::sourceSigned, ConverterS<To, From>,
                                ConverterU<To, From> >::type,
      typename std::conditional<Info::sourceSigned, ConverterSU<To, From>,
                                ConverterUS<To, From> >::type>::type Conv;

  static inline To convert(From value) { return Conv::convert(value); }
};
}  // namespace Intern

template <typename To, typename From>
inline To checked_cast(From value) {
  return Intern::CheckedConverter<To, From>::convert(value);
}

template <typename T>
T checked_add(T v1, T v2) {
  if (v1 >= 0 && v2 >= 0) {
    if (std::numeric_limits<T>::max() - v1 >= v2) return v1 + v2;
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Overflow during addition");
  } else if (v1 < 0 && v2 < 0) {
    if (std::numeric_limits<T>::min() - v1 <= v2) return v1 + v2;
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Overflow during addition");
  } else {
    return v1 + v2;  // When one number is positive and one number is negative
                     // there will be no overflow
  }
}

template <typename T>
T checked_mul(T v1, T v2) {
  // Multiplication with 0 or 1 always works
  if (v1 == 0 || v1 == 1 || v2 == 0 || v2 == 1) return v1 * v2;
  if (v1 < 0 && v1 == std::numeric_limits<T>::min()) {
    // Signed min values can only be multiplied with 0 and 1 (and not with -1)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Overflow during multiplication");
  }
  if (v2 < 0 && v2 == std::numeric_limits<T>::min()) {
    // Signed min values can only be multiplied with 0 and 1 (and not with -1)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Overflow during multiplication");
  }
  typedef typename std::make_unsigned<T>::type uT;
  uT v1A = checked_cast<uT>(v1 >= 0 ? v1 : -v1);
  uT v2A = checked_cast<uT>(v2 >= 0 ? v2 : -v2);
  if (std::numeric_limits<T>::max() / v1A >= v2A) return v1 * v2;
  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Overflow during multiplication");
}

template <typename T>
class Array1 {
  typedef typename std::remove_cv<T>::type TnoCV;
  typedef
      typename std::conditional<std::is_const<T>::value, const char, char>::type
          MaybeConstChar;
  QSharedPointer<int> backend;
  T* ptr;
  std::array<size_t, 1> shape;
  std::array<ptrdiff_t, 1> stride;

  template <typename BackendPtr>
  static QSharedPointer<int> getBackend(const QSharedPointer<BackendPtr>& ptr) {
    // Note: Do not pass nullptr to QSharedPointer as otherwise the deleter
    // won't get called, see https://bugreports.qt.io/browse/QTBUG-85285
    return QSharedPointer<int>((int*)1,
                               // This deleter works by keeping ptr alive until
                               // the returned pointer is destroyed
                               [ptr](int*) {});
  }

 public:
  Array1(const vx::Array1Info& info) {
    bool writable = !std::is_const<T>::value;

    QString byteorder;
    if (sizeof(T) == 1) {
      byteorder = "none";
    } else {
      if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
        byteorder = "big";
      } else if (QSysInfo::Endian::ByteOrder ==
                 QSysInfo::Endian::LittleEndian) {
        byteorder = "little";
      } else {
        byteorder = "unknown";
      }
    }

    if (info.dataType != ArrayTypeInfo<TnoCV>::name())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          QString() + "Unexpected data type: Expected '" +
                              ArrayTypeInfo<TnoCV>::name() + "', got '" +
                              info.dataType + "'");
    if (info.dataTypeSize != sizeof(T) * 8)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          QString("Unexpected data type size: Expected %1, got %2")
              .arg(sizeof(T) * 8, info.dataTypeSize));
    if (info.byteorder != byteorder)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          QString("Unexpected data type byte order: Expected %1, got %2")
              .arg(byteorder, info.byteorder));

    shape[0] = vx::checked_cast<size_t>(info.size);
    /*
    size_t bytesOverall = shape[0] * sizeof(T);
    if (bytesOverall / shape[0] != sizeof(T))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                   "Overflow while calculating size");
    */

    this->stride[0] = vx::checked_cast<ptrdiff_t>(info.stride);

    auto bytes = vx::checked_cast<int64_t>(info.offset);
    auto pos0 = vx::checked_cast<int64_t>(info.offset);
    bytes =
        vx::checked_add<int64_t>(bytes, vx::checked_cast<int64_t>(sizeof(T)));
    for (int i = 0; i < 1; i++) {
      if (shape[i] == 0) {
        bytes = 0;
        pos0 = 0;
        break;
      }
      auto sizeDim = vx::checked_mul<int64_t>(
          (vx::checked_cast<int64_t>(shape[i] - 1)), stride[i]);
      if (stride[i] > 0)
        bytes = vx::checked_add<int64_t>(bytes, sizeDim);
      else
        pos0 = vx::checked_add<int64_t>(pos0, sizeDim);
    }
    // qDebug() << pos0 << bytes << shape[0] * shape[1] * sizeof(T);

    ptrdiff_t offset = vx::checked_cast<ptrdiff_t>(info.offset);
    if (offset < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "offset < 0");
    if (pos0 < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "pos0 < 0");

    auto buffer =
        createQSharedPointer<vx::MappedBuffer>(info.handle, bytes, writable);
    backend = getBackend(buffer);

    ptr = (T*)(((char*)buffer->data()) + offset);
  }

  template <typename BackendPtr>
  Array1(T* ptr, const std::array<size_t, 1>& shape,
         const std::array<ptrdiff_t, 1>& stride,
         const QSharedPointer<BackendPtr>& backend)
      : backend(getBackend(backend)), ptr(ptr), shape(shape), stride(stride) {}

  Array1(const std::array<size_t, 1>& shape) {
    size_t bytesOverall = sizeof(T);
    for (int i = 0; i < 1; i++) {
      this->stride[i] = bytesOverall;
      this->shape[i] = shape[i];
      bytesOverall = vx::checked_mul<size_t>(bytesOverall, shape[i]);
    }

    QSharedPointer<T> sharedPtr(new T[bytesOverall / sizeof(T)],
                                [](T* ptr) { delete[] ptr; });
    backend = getBackend(sharedPtr);
    ptr = sharedPtr.data();
  }

  T* data() const { return ptr; }

  QSharedPointer<int> getBackend() const { return backend; }

  T& operator()(size_t x) const {
    /*
    qDebug() << "acc" << x << y << shape[0] << shape[1] << stride[0]
             << stride[1];
    */
    if (x >= shape[0])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bound array access");
    return *(T*)(((MaybeConstChar*)data()) + (x * stride[0]));
  }

  template <size_t dim>
  size_t size() const {
    static_assert(dim < 1, "dim < 1");
    return std::get<dim>(shape);
  }

  template <size_t dim>
  ptrdiff_t strideBytes() const {
    static_assert(dim < 1, "dim < 1");
    return std::get<dim>(stride);
  }
};

template <typename T>
class Array2 {
  typedef typename std::remove_cv<T>::type TnoCV;
  typedef
      typename std::conditional<std::is_const<T>::value, const char, char>::type
          MaybeConstChar;
  QSharedPointer<int> backend;
  T* ptr;
  std::array<size_t, 2> shape;
  std::array<ptrdiff_t, 2> stride;

  template <typename BackendPtr>
  static QSharedPointer<int> getBackend(const QSharedPointer<BackendPtr>& ptr) {
    // Note: Do not pass nullptr to QSharedPointer as otherwise the deleter
    // won't get called, see https://bugreports.qt.io/browse/QTBUG-85285
    return QSharedPointer<int>((int*)1,
                               // This deleter works by keeping ptr alive until
                               // the returned pointer is destroyed
                               [ptr](int*) {});
  }

 public:
  Array2(const vx::Array2Info& info) {
    bool writable = !std::is_const<T>::value;

    QString byteorder;
    if (sizeof(T) == 1) {
      byteorder = "none";
    } else {
      if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
        byteorder = "big";
      } else if (QSysInfo::Endian::ByteOrder ==
                 QSysInfo::Endian::LittleEndian) {
        byteorder = "little";
      } else {
        byteorder = "unknown";
      }
    }

    if (info.dataType != ArrayTypeInfo<TnoCV>::name())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          QString() + "Unexpected data type: Expected '" +
                              ArrayTypeInfo<TnoCV>::name() + "', got '" +
                              info.dataType + "'");
    if (info.dataTypeSize != sizeof(T) * 8)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          QString("Unexpected data type size: Expected %1, got %2")
              .arg(sizeof(T) * 8, info.dataTypeSize));
    if (info.byteorder != byteorder)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          QString("Unexpected data type byte order: Expected %1, got %2")
              .arg(byteorder, info.byteorder));

    shape[0] = vx::checked_cast<size_t>(info.sizeX);
    shape[1] = vx::checked_cast<size_t>(info.sizeY);
    /*
    size_t bytesOverall = shape[0] * shape[1] * sizeof(T);
    if (bytesOverall / shape[0] / shape[1] != sizeof(T))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                   "Overflow while calculating size");
    */

    this->stride[0] = vx::checked_cast<ptrdiff_t>(info.strideX);
    this->stride[1] = vx::checked_cast<ptrdiff_t>(info.strideY);

    auto bytes = vx::checked_cast<int64_t>(info.offset);
    auto pos0 = vx::checked_cast<int64_t>(info.offset);
    bytes =
        vx::checked_add<int64_t>(bytes, vx::checked_cast<int64_t>(sizeof(T)));
    for (int i = 0; i < 2; i++) {
      if (shape[i] == 0) {
        bytes = 0;
        pos0 = 0;
        break;
      }
      auto sizeDim = vx::checked_mul<int64_t>(
          (vx::checked_cast<int64_t>(shape[i] - 1)), stride[i]);
      if (stride[i] > 0)
        bytes = vx::checked_add<int64_t>(bytes, sizeDim);
      else
        pos0 = vx::checked_add<int64_t>(pos0, sizeDim);
    }
    // qDebug() << pos0 << bytes << shape[0] * shape[1] * sizeof(T);

    ptrdiff_t offset = vx::checked_cast<ptrdiff_t>(info.offset);
    if (offset < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "offset < 0");
    if (pos0 < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "pos0 < 0");

    auto buffer =
        createQSharedPointer<vx::MappedBuffer>(info.handle, bytes, writable);
    backend = getBackend(buffer);

    ptr = (T*)(((char*)buffer->data()) + offset);
  }

  template <typename BackendPtr>
  Array2(T* ptr, const std::array<size_t, 2>& shape,
         const std::array<ptrdiff_t, 2>& stride,
         const QSharedPointer<BackendPtr>& backend)
      : backend(getBackend(backend)), ptr(ptr), shape(shape), stride(stride) {}

  Array2(const std::array<size_t, 2>& shape) {
    size_t bytesOverall = sizeof(T);
    for (int i = 0; i < 2; i++) {
      this->stride[i] = bytesOverall;
      this->shape[i] = shape[i];
      bytesOverall = vx::checked_mul<size_t>(bytesOverall, shape[i]);
    }

    QSharedPointer<T> sharedPtr(new T[bytesOverall / sizeof(T)],
                                [](T* ptr) { delete[] ptr; });
    backend = getBackend(sharedPtr);
    ptr = sharedPtr.data();
  }

  T* data() const { return ptr; }

  QSharedPointer<int> getBackend() const { return backend; }

  T& operator()(size_t x, size_t y) const {
    /*
    qDebug() << "acc" << x << y << shape[0] << shape[1] << stride[0]
             << stride[1];
    */
    if (x >= shape[0] || y >= shape[1])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bound array access");
    return *(T*)(((MaybeConstChar*)data()) + (x * stride[0]) + (y * stride[1]));
  }

  template <size_t dim>
  size_t size() const {
    static_assert(dim < 2, "dim < 2");
    return std::get<dim>(shape);
  }

  template <size_t dim>
  ptrdiff_t strideBytes() const {
    static_assert(dim < 2, "dim < 2");
    return std::get<dim>(stride);
  }
};

template <typename T>
class Array3 {
  // TODO: Should not be needed, exposes via accessors
  friend class Array3<void>;
  friend class Array3<const void>;

  typedef typename std::remove_cv<T>::type TnoCV;
  typedef
      typename std::conditional<std::is_const<T>::value, const char, char>::type
          MaybeConstChar;
  QSharedPointer<int> backend;
  T* ptr;
  std::array<size_t, 3> shape;
  std::array<ptrdiff_t, 3> stride;

  template <typename BackendPtr>
  static QSharedPointer<int> getBackend(const QSharedPointer<BackendPtr>& ptr) {
    // Note: Do not pass nullptr to QSharedPointer as otherwise the deleter
    // won't get called, see https://bugreports.qt.io/browse/QTBUG-85285
    return QSharedPointer<int>((int*)1,
                               // This deleter works by keeping ptr alive until
                               // the returned pointer is destroyed
                               [ptr](int*) {});
  }

  struct DummyStruct {};

 public:
  Array3(const vx::Array3Info& info) {
    bool writable = !std::is_const<T>::value;

    QString byteorder;
    if (sizeof(T) == 1) {
      byteorder = "none";
    } else {
      if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
        byteorder = "big";
      } else if (QSysInfo::Endian::ByteOrder ==
                 QSysInfo::Endian::LittleEndian) {
        byteorder = "little";
      } else {
        byteorder = "unknown";
      }
    }

    if (info.dataType != ArrayTypeInfo<TnoCV>::name())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          QString() + "Unexpected data type: Expected '" +
                              ArrayTypeInfo<TnoCV>::name() + "', got '" +
                              info.dataType + "'");
    if (info.dataTypeSize != sizeof(T) * 8)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          QString("Unexpected data type size: Expected %1, got %2")
              .arg(sizeof(T) * 8, info.dataTypeSize));
    if (info.byteorder != byteorder)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          QString("Unexpected data type byte order: Expected %1, got %2")
              .arg(byteorder, info.byteorder));

    shape[0] = vx::checked_cast<size_t>(info.sizeX);
    shape[1] = vx::checked_cast<size_t>(info.sizeY);
    shape[2] = vx::checked_cast<size_t>(info.sizeZ);
    /*
    size_t bytesOverall = shape[0] * shape[1] *  shape[3] * sizeof(T);
    if (bytesOverall / shape[0] / shape[1] / shape[2] != sizeof(T))
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                   "Overflow while calculating size");
    */

    this->stride[0] = vx::checked_cast<ptrdiff_t>(info.strideX);
    this->stride[1] = vx::checked_cast<ptrdiff_t>(info.strideY);
    this->stride[2] = vx::checked_cast<ptrdiff_t>(info.strideZ);

    auto bytes = vx::checked_cast<int64_t>(info.offset);
    auto pos0 = vx::checked_cast<int64_t>(info.offset);
    bytes =
        vx::checked_add<int64_t>(bytes, vx::checked_cast<int64_t>(sizeof(T)));
    for (int i = 0; i < 3; i++) {
      if (shape[i] == 0) {
        bytes = 0;
        pos0 = 0;
        break;
      }
      auto sizeDim = vx::checked_mul<int64_t>(
          (vx::checked_cast<int64_t>(shape[i] - 1)), stride[i]);
      if (stride[i] > 0)
        bytes = vx::checked_add<int64_t>(bytes, sizeDim);
      else
        pos0 = vx::checked_add<int64_t>(pos0, sizeDim);
    }
    // qDebug() << pos0 << bytes << shape[0] * shape[1] * shape[2] * sizeof(T);

    ptrdiff_t offset = vx::checked_cast<ptrdiff_t>(info.offset);
    if (offset < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "offset < 0");
    if (pos0 < 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "pos0 < 0");

    auto buffer =
        createQSharedPointer<vx::MappedBuffer>(info.handle, bytes, writable);
    backend = getBackend(buffer);

    ptr = (T*)(((char*)buffer->data()) + offset);
  }

  explicit inline Array3(const Array3<void>& av);
  // This constructor is only available if T is const
  // template <typename U = std::enable_if_t<std::is_const<T>::value, T>>
  // explicit inline Array3(const Array3<const void>& av);
  template <typename U = T>
  explicit inline Array3(
      const Array3<const void>& av,
      std::enable_if_t<std::is_const<U>::value, DummyStruct> = DummyStruct());

  template <typename BackendPtr>
  Array3(T* ptr, const std::array<size_t, 3>& shape,
         const std::array<ptrdiff_t, 3>& stride,
         const QSharedPointer<BackendPtr>& backend)
      : backend(getBackend(backend)), ptr(ptr), shape(shape), stride(stride) {}

  Array3(const std::array<size_t, 3>& shape) {
    size_t bytesOverall = sizeof(T);
    for (int i = 0; i < 3; i++) {
      this->stride[i] = bytesOverall;
      this->shape[i] = shape[i];
      bytesOverall = vx::checked_mul<size_t>(bytesOverall, shape[i]);
    }

    QSharedPointer<T> sharedPtr(new T[bytesOverall / sizeof(T)],
                                [](T* ptr) { delete[] ptr; });
    backend = getBackend(sharedPtr);
    ptr = sharedPtr.data();
  }

  T* data() const { return ptr; }

  QSharedPointer<int> getBackend() const { return backend; }

  T& operator()(size_t x, size_t y, size_t z) const {
    /*
    qDebug() << "acc" << x << y << shape[0] << shape[1] <<  shape[2]
             << stride[0] << stride[1] << stride[2];
    */
    if (x >= shape[0] || y >= shape[1] || z >= shape[2])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bound array access");
    return *(T*)(((MaybeConstChar*)data()) + (x * stride[0]) + (y * stride[1]) +
                 (z * stride[2]));
  }

  template <size_t dim>
  size_t size() const {
    static_assert(dim < 3, "dim < 3");
    return std::get<dim>(shape);
  }

  template <size_t dim>
  ptrdiff_t strideBytes() const {
    static_assert(dim < 3, "dim < 3");
    return std::get<dim>(stride);
  }
};

// TODO: Merge partially with Array3<T>?
template <>
class Array3<void> {
  // TODO: Should not be needed, exposes via accessors
  template <typename T>
  friend class Array3;

  typedef void T;
  typedef char MaybeConstChar;

  QSharedPointer<int> backend;
  T* ptr;
  std::array<size_t, 3> shape;
  std::array<ptrdiff_t, 3> stride;

  template <typename BackendPtr>
  static QSharedPointer<int> getBackend(const QSharedPointer<BackendPtr>& ptr) {
    // Note: Do not pass nullptr to QSharedPointer as otherwise the deleter
    // won't get called, see https://bugreports.qt.io/browse/QTBUG-85285
    return QSharedPointer<int>((int*)1,
                               // This deleter works by keeping ptr alive until
                               // the returned pointer is destroyed
                               [ptr](int*) {});
  }

 public:
  template <typename BackendPtr>
  Array3(T* ptr, const std::array<size_t, 3>& shape,
         const std::array<ptrdiff_t, 3>& stride,
         const QSharedPointer<BackendPtr>& backend)
      : backend(getBackend(backend)), ptr(ptr), shape(shape), stride(stride) {}

  template <typename U>
  explicit Array3(const Array3<U>& a)
      : backend(a.backend), ptr(a.ptr), shape(a.shape), stride(a.stride) {}

  T* data() const { return ptr; }

  QSharedPointer<int> getBackend() const { return backend; }

  template <size_t dim>
  size_t size() const {
    static_assert(dim < 3, "dim < 3");
    return std::get<dim>(shape);
  }

  template <size_t dim>
  ptrdiff_t strideBytes() const {
    static_assert(dim < 3, "dim < 3");
    return std::get<dim>(stride);
  }
};

// TODO: Clean up, avoid separate implementation for const void?
template <>
class Array3<const void> {
  // TODO: Should not be needed, exposes via accessors
  template <typename T>
  friend class Array3;

  typedef const void T;
  typedef const char MaybeConstChar;

  QSharedPointer<int> backend;
  T* ptr;
  std::array<size_t, 3> shape;
  std::array<ptrdiff_t, 3> stride;

  template <typename BackendPtr>
  static QSharedPointer<int> getBackend(const QSharedPointer<BackendPtr>& ptr) {
    // Note: Do not pass nullptr to QSharedPointer as otherwise the deleter
    // won't get called, see https://bugreports.qt.io/browse/QTBUG-85285
    return QSharedPointer<int>((int*)1,
                               // This deleter works by keeping ptr alive until
                               // the returned pointer is destroyed
                               [ptr](int*) {});
  }

 public:
  template <typename BackendPtr>
  Array3(T* ptr, const std::array<size_t, 3>& shape,
         const std::array<ptrdiff_t, 3>& stride,
         const QSharedPointer<BackendPtr>& backend)
      : backend(getBackend(backend)), ptr(ptr), shape(shape), stride(stride) {}

  template <typename U>
  explicit Array3(const Array3<U>& a)
      : backend(a.backend), ptr(a.ptr), shape(a.shape), stride(a.stride) {}

  // Allow (implicit) conversion void->const void
  Array3(const Array3<void>& av)
      : backend(av.backend),
        ptr((T*)av.ptr),
        shape(av.shape),
        stride(av.stride) {}

  T* data() const { return ptr; }

  QSharedPointer<int> getBackend() const { return backend; }

  template <size_t dim>
  size_t size() const {
    static_assert(dim < 3, "dim < 3");
    return std::get<dim>(shape);
  }

  template <size_t dim>
  ptrdiff_t strideBytes() const {
    static_assert(dim < 3, "dim < 3");
    return std::get<dim>(stride);
  }
};

template <typename T>
inline Array3<T>::Array3(const Array3<void>& av)
    : backend(av.backend),
      ptr((T*)av.ptr),
      shape(av.shape),
      stride(av.stride) {}
// This constructor is only available if T is const
template <typename T>
template <typename U>
inline Array3<T>::Array3(const Array3<const void>& av,
                         std::enable_if_t<std::is_const<U>::value, DummyStruct>)
    : backend(av.backend),
      ptr((const T*)av.ptr),
      shape(av.shape),
      stride(av.stride) {
  static_assert(std::is_const<U>::value,
                "This constructor is only available if T is const");
  static_assert(std::is_same<T, U>::value, "T and U must be the same");
}
}  // namespace vx
