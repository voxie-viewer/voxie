/*
 * Copyright (c) 2014-2025 The Voxie Authors
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

#include <QtCore/QMap>

#include <vector>

namespace vx {
// QList::toStdVector() was deprecated in Qt 5.14, but in many cases using this
// is much easier than calling the std::vector constructor with two arguments.
template <typename T>
std::vector<T> toStdVector(const QVector<T>& vector) {
  return std::vector<T>(vector.begin(), vector.end());
}
template <typename T>
std::vector<T> toStdVector(const QList<T>& list) {
  return std::vector<T>(list.begin(), list.end());
}

// QVector::fromStdVector() was deprecated in Qt 5.14, but the replacement is
// not available before Qt 5.14.
template <typename T>
QVector<T> toQVector(const std::vector<T>& vector) {
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  return QVector<T>::fromStdVector(vector);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}
template <typename T>
QList<T> toQList(const std::vector<T>& vector) {
  return toQVector(vector).toList();
}

// QSet::fromList() was deprecated in Qt 5.14, but the replacement is
// not available before Qt 5.14.
template <typename T>
QSet<T> toQSet(const QList<T>& list) {
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  return QSet<T>::fromList(list);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

namespace Intern {
VOXIECLIENT_EXPORT Q_NORETURN void throwValueNotFound();
}

// Get a value from a QMap, throw on error.
// Works even if V does not have a default ctor.
template <typename K, typename V>
static inline const V& getQMapValue(const QMap<K, V>& map, const K& key) {
  auto it = map.find(key);
  if (it == map.end())
    // TODO: Provide more information, try to turn key into a string?
    Intern::throwValueNotFound();
  else
    return it.value();
}
}  // namespace vx
