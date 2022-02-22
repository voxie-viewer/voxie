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

#include "ThreadSafe_MxN_Matrix.hpp"

ThreadSafe_MxN_Matrix::ThreadSafe_MxN_Matrix(QObject* parent, size_t rowCount,
                                             size_t columnCount)
    : QObject(parent) {
  this->internData = std::vector<float>();
  this->rowCount = rowCount;
  this->columnCount = columnCount;
  internData.resize(this->rowCount * this->columnCount);
}

float* ThreadSafe_MxN_Matrix::getData() {
  QMutexLocker locker(&this->mutex);

  return this->internData.data();
}

void ThreadSafe_MxN_Matrix::addValue(float value, size_t x, size_t y) {
  QMutexLocker locker(&this->mutex);

  this->internData[y * this->columnCount + x] = value;
}
float ThreadSafe_MxN_Matrix::getValue(size_t row, size_t column) {
  QMutexLocker locker(&this->mutex);

  return this->internData.at((row * this->columnCount) + column);
}
size_t ThreadSafe_MxN_Matrix::size() {
  QMutexLocker locker(&this->mutex);

  return this->internData.size();
}

void ThreadSafe_MxN_Matrix::resize(size_t rowCount, size_t columnCount) {
  QMutexLocker locker(&this->mutex);

  this->rowCount = rowCount;
  this->columnCount = columnCount;
  this->internData.resize(this->rowCount * this->columnCount, 0);
}
