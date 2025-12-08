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

#include "TensorComponent.hpp"

#include <VoxieClient/Format.hpp>

vx::TensorComponent::TensorComponent(const QList<quint32>& contravariantIndices,
                                     const QList<quint32>& covariantIndices)
    : contravariantIndices_(contravariantIndices),
      covariantIndices_(covariantIndices) {}
vx::TensorComponent::~TensorComponent() {}

vx::TensorComponent vx::TensorComponent::parse(
    const std::tuple<QList<quint32>, QList<quint32>>& raw) {
  return TensorComponent(std::get<0>(raw), std::get<1>(raw));
}
std::tuple<QList<quint32>, QList<quint32>> vx::TensorComponent::toRaw() const {
  return std::make_tuple(contravariantIndices(), covariantIndices());
}

// TODO: Move somewhere else?
// TODO: Implement a formatter? (Should the formatter abbreviate long lists?)
template <typename T>
static QString myToList(const QList<T>& list) {
  QString res = "[";
  for (int i = 0; i < list.size(); i++) {
    if (i) res += ", ";
    res += QString::number(list[i]);
  }
  res += "]";
  return res;
}

QString vx::TensorComponent::toString() const {
  return vx::format("Tensor type ({}, {}): {} / {}", contravariantOrder(),
                    covariantOrder(), myToList(contravariantIndices()),
                    myToList(covariantIndices()));
}
QString vx::TensorComponent::description() const {
  // TODO: Return something different?
  return toString();
}

vx::TensorComponent vx::TensorComponent::scalar() {
  return TensorComponent({}, {});
}

namespace vx {
bool operator==(TensorComponent t1, TensorComponent t2) {
  return t1.contravariantIndices() == t2.contravariantIndices() &&
         t1.covariantIndices() == t2.covariantIndices();
}
bool operator!=(TensorComponent t1, TensorComponent t2) { return !(t1 == t2); }
bool operator<(TensorComponent t1, TensorComponent t2) {
  if (t1.totalOrder() < t2.totalOrder()) return true;
  if (t1.totalOrder() > t2.totalOrder()) return false;

  if (t1.contravariantOrder() < t2.contravariantOrder()) return true;
  if (t1.contravariantOrder() > t2.contravariantOrder()) return false;

  for (quint32 i = 0; i < t1.contravariantOrder(); i++) {
    if (t1.contravariantIndices()[i] < t2.contravariantIndices()[i])
      return true;
    if (t1.contravariantIndices()[i] > t2.contravariantIndices()[i])
      return false;
  }

  for (quint32 i = 0; i < t1.covariantOrder(); i++) {
    if (t1.covariantIndices()[i] < t2.covariantIndices()[i]) return true;
    if (t1.covariantIndices()[i] > t2.covariantIndices()[i]) return false;
  }

  return false;
}
bool operator>(TensorComponent t1, TensorComponent t2) { return t2 < t1; }
bool operator<=(TensorComponent t1, TensorComponent t2) { return !(t1 > t2); }
bool operator>=(TensorComponent t1, TensorComponent t2) { return !(t1 < t2); }
}  // namespace vx
