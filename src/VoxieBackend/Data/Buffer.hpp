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

#include <VoxieBackend/Data/SharedMemory.hpp>

#include <VoxieClient/Array.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

namespace vx {
class BufferType;
class BufferTypeBase;
class SharedMemory;

class VOXIEBACKEND_EXPORT Buffer : public RefCountedObject {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  // QSharedPointer<BufferType> elementBufferType_;
  QSharedPointer<BufferTypeBase> elementType_;
  quint64 count_;
  qint64 strideBytes_;
  QSharedPointer<SharedMemory> data_;

 public:
  Buffer(const QSharedPointer<BufferTypeBase>& elementType, quint64 count,
         qint64 strideBytes);
  ~Buffer();

  // const QSharedPointer<BufferType>& elementBufferType() { return
  // elementBufferType_; }
  const QSharedPointer<BufferTypeBase>& elementType() { return elementType_; }
  quint64 count() { return count_; }
  qint64 strideBytes() { return strideBytes_; }
  const QSharedPointer<SharedMemory>& data() { return data_; }

  QSharedPointer<BufferTypeBase> type();

 private:
  void checkTypeAndStrides(const QSharedPointer<BufferType>& expectedType);

 public:
  template <typename T>
  // vx::Array1<typename T::Type>
  vx::Array1<typename std::conditional<
      std::is_const<T>::value, const typename T::Type, typename T::Type>::type>
  asArray1() {
    constexpr bool isConst = std::is_const<T>::value;
    using T_ = typename std::remove_cv<T>::type;
    using Type_ = typename T_::Type;
    using Type = typename std::conditional<isConst, const Type_, Type_>::type;

    checkTypeAndStrides(T_::bufferType());

    return vx::Array1<Type>((Type*)data()->getData(), {this->count()},
                            {this->strideBytes()}, this->thisShared());
  }
};
}  // namespace vx
