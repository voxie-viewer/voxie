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

#include <VoxieBackend/Data/SharedMemory.hpp>

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <vector>

#include <QtCore/QSharedPointer>

namespace vx {

// This is not marked as VOXIEBACKEND_EXPORT because it is header-only and
// Windows would want explicit instantiations for this template otherwise.
// (Seems like Q_DECL_IMPORT prevents the compiler from instantiating functions
// unless they are implemented inline.)
template <typename T>
class SharedMemoryArray {
  size_t count_;
  QSharedPointer<SharedMemory> shared;

  static size_t getNeededMemory(size_t count) {
    size_t needed = count * sizeof(T);
    if (needed / sizeof(T) != count)
      throw vx::Exception("de.uni_stuttgart.Voxie.OutOfMemory",
                          "Overflow while calculating size");
    return needed;
  }

 public:
  explicit SharedMemoryArray(size_t count)
      : count_(count),
        shared(createQSharedPointer<SharedMemory>(getNeededMemory(count))) {}

  const QSharedPointer<SharedMemory>& getSharedMemory() { return shared; }

  size_t size() const { return count_; }

  T* data() { return (T*)shared->getData(); }
  const T* data() const { return (const T*)shared->getData(); }

  // TODO: somehow allow a SharedMemoryArray to be passed to a std::vector ctor?
  void writeToVector(std::vector<T>& vec) const {
    vec.resize(size());
    // TODO: faster?
    for (size_t i = 0; i < size(); i++) vec[i] = data()[i];
  }

  // TODO: version without bounds checks?
  const T& operator[](size_t i) const {
    if (i >= size())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bounds SharedMemoryArray<> access");
    return *(data() + i);
  }
  T& operator[](size_t i) {
    if (i >= size())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bounds SharedMemoryArray<> access");
    return *(data() + i);
  }

  T* begin() { return data(); }
  T* end() { return data() + size(); }
  const T* begin() const { return data(); }
  const T* end() const { return data() + size(); }
};

}  // namespace vx
