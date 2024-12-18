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

#include <VoxieBackend/Data/Data.hpp>

#include <memory>

namespace vx {
class GeometricPrimitive;

class VOXIEBACKEND_EXPORT GeometricPrimitiveData : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 private:
  QMap<quint64, QSharedPointer<GeometricPrimitive>> primitives_;
  quint64 lastId = 0;

 public:
  // throws Exception
  // TODO: should this be private so that only the create() method can call it?
  GeometricPrimitiveData();

 public:
  ~GeometricPrimitiveData();

  QList<QString> supportedDBusInterfaces() override;

  QList<quint64> getPrimitiveIDs();
  const QMap<quint64, QSharedPointer<GeometricPrimitive>>& primitives() {
    return primitives_;
  }

  QSharedPointer<GeometricPrimitive> getPrimitive(quint64 id);
  QSharedPointer<GeometricPrimitive> getPrimitiveOrNull(quint64 id);

  quint64 addPrimitive(const QSharedPointer<DataUpdate>& update,
                       const QSharedPointer<GeometricPrimitive>& primitive);

  void addOrReplacePrimitive(
      const QSharedPointer<DataUpdate>& update, quint64 id,
      const QSharedPointer<GeometricPrimitive>& newPrimitive);

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};
}  // namespace vx
