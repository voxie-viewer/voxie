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

#include <Voxie/Voxie.hpp>

#include <QtCore/QList>

#include <tuple>

namespace vx {
class VOXIECORESHARED_EXPORT TensorComponent {
  QList<quint32> contravariantIndices_;
  QList<quint32> covariantIndices_;

 public:
  explicit TensorComponent(const QList<quint32>& contravariantIndices,
                           const QList<quint32>& covariantIndices);
  ~TensorComponent();

  const QList<quint32>& contravariantIndices() const {
    return contravariantIndices_;
  }
  const QList<quint32>& covariantIndices() const { return covariantIndices_; }

  quint32 contravariantOrder() const { return contravariantIndices().size(); }
  quint32 covariantOrder() const { return covariantIndices().size(); }
  quint32 totalOrder() const {
    return contravariantIndices().size() + covariantIndices().size();
  }

  static TensorComponent parse(
      const std::tuple<QList<quint32>, QList<quint32>>& raw);
  std::tuple<QList<quint32>, QList<quint32>> toRaw() const;

  QString toString() const;
  QString description() const;

  static TensorComponent scalar();

  friend bool operator==(TensorComponent t1, TensorComponent t2);
  friend bool operator!=(TensorComponent t1, TensorComponent t2);
  friend bool operator<(TensorComponent t1, TensorComponent t2);
  friend bool operator>(TensorComponent t1, TensorComponent t2);
  friend bool operator<=(TensorComponent t1, TensorComponent t2);
  friend bool operator>=(TensorComponent t1, TensorComponent t2);
};
}  // namespace vx
