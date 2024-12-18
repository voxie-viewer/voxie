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

#include <VoxieClient/Vector.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <QtGui/QVector3D>

#include <QtDBus/QDBusVariant>

namespace vx {
class GeometricPrimitiveType;

class VOXIEBACKEND_EXPORT GeometricPrimitive {
  QString name_;

 public:
  GeometricPrimitive(const QString& name);
  virtual ~GeometricPrimitive();

  virtual QSharedPointer<GeometricPrimitive> withName(const QString& name) = 0;

  virtual QSharedPointer<GeometricPrimitiveType> primitiveType() = 0;

  virtual QMap<QString, QDBusVariant> primitiveValues() = 0;

  const QString& name() const { return name_; }

  static QSharedPointer<QMap<QString, QSharedPointer<GeometricPrimitiveType>>>
  allTypes();
};

class VOXIEBACKEND_EXPORT GeometricPrimitivePoint : public GeometricPrimitive {
  vx::Vector<double, 3> position_;

 public:
  // TODO: Remove
  GeometricPrimitivePoint(const QString& name, const QVector3D& position);
  GeometricPrimitivePoint(const QString& name,
                          const vx::Vector<double, 3>& position);
  ~GeometricPrimitivePoint() override;

  QSharedPointer<GeometricPrimitive> withName(const QString& name) override;

  QSharedPointer<GeometricPrimitiveType> primitiveType() override;

  QMap<QString, QDBusVariant> primitiveValues() override;

  const vx::Vector<double, 3> positionVec() const { return position_; }
  // TODO: Remove, replace with positionVec()
  QVector3D position() const;

  static QSharedPointer<GeometricPrimitive> create(
      const QString& name, const QMap<QString, QDBusVariant>& primitiveValues);

  static QSharedPointer<GeometricPrimitiveType> type();
};
}  // namespace vx
