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

#include <cstring>

#include <QtCore/QDebug>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>

// TODO: Merge FloatImage and FloatBuffer into Image?
class VOXIEBACKEND_EXPORT FloatBuffer {
 public:
  // throws Exception if enableSharedMemory is true
  FloatBuffer(size_t numElements, bool enableSharedMemory,
              bool enableSharedMemoryWrite);

  inline size_t numElements() const { return this->_numElements; }
  inline size_t length() const { return this->_numElements; }
  inline size_t size() const { return this->_numElements; }
  inline size_t byteSize() const { return this->_numElements * sizeof(float); }
  inline bool isEmpty() const { return this->_numElements == 0; }

  inline const float* data() const { return this->_values; }
  inline float* data() { return this->_values; }
  inline float at(size_t index) const {
    Q_ASSERT(index < _numElements);
    return this->data()[index];
  }
  inline void setElement(size_t index, float element) {
    Q_ASSERT(index < _numElements);
    this->data()[index] = element;
  }
  inline float& operator[](size_t i) {
    Q_ASSERT(i < _numElements);
    return this->data()[i];
  }
  inline const float& operator[](size_t i) const {
    Q_ASSERT(i < _numElements);
    return this->data()[i];
  }

  QSharedPointer<vx::SharedMemory> getSharedMemory() { return sharedMemory_; }

  inline QVector<float> toQVector() const {
    QVector<float> vec(
        (int)this->numElements());  // QVector maximum size is int
    if (this->_numElements > 0)
      memcpy(vec.data(), this->data(), this->byteSize());
    return vec;
  }
  // throws Exception if enableSharedMemory is true
  inline FloatBuffer copy(bool enableSharedMemory,
                          bool enableSharedMemoryWrite = false) const {
    FloatBuffer bufferCopy(this->_numElements, enableSharedMemory,
                           enableSharedMemoryWrite);
    if (this->_numElements > 0)
      memcpy(bufferCopy.data(), this->data(), this->byteSize());
    return bufferCopy;
  }

 private:
  float* _values;
  QSharedPointer<float> buffer_;
  QSharedPointer<vx::SharedMemory> sharedMemory_;
  size_t _numElements;

  template <typename T_>
  static void deleteArray(T_ array[]) {
    delete[] array;
  }
};
