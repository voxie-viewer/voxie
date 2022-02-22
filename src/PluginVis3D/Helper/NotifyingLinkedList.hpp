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

#include <PluginVis3D/Helper/NotifyingList.hpp>
#include <QLinkedList>
#include <cstddef>

template <typename T>
class NotifyingLinkedList : public NotifyingList {
 private:
  QLinkedList<T> list = QLinkedList<T>();

 public:
  typename QLinkedList<T>::iterator begin() { return this->list.begin(); }

  typename QLinkedList<T>::iterator end() { return this->list.end(); }

  void add(T value) {
    this->list.append(value);
    Q_EMIT listChanged();
  }

  void clear() {
    this->list.clear();
    Q_EMIT listChanged();
  }

  bool isEmpty() { return this->list.empty(); }

  T removeFirst() {
    Q_EMIT listChanged();
    return this->list.removeFirst();
  }

  bool remove(T value) {
    auto removed = this->list.removeOne(value);
    if (removed) {
      Q_EMIT listChanged();
    }
    return removed;
  }

  bool contains(T value) { return this->list.contains(value); }
};
