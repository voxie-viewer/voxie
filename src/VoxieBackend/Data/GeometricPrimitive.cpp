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

#include "GeometricPrimitive.hpp"

#include <Voxie/MathQt.hpp>

#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <VoxieBackend/Data/GeometricPrimitiveType.hpp>

using namespace vx;

GeometricPrimitive::GeometricPrimitive(const QString& name) : name_(name) {}
GeometricPrimitive::~GeometricPrimitive() {}

static QSharedPointer<QMap<QString, QSharedPointer<GeometricPrimitiveType>>>
createAllTypes() {
  auto res = createQSharedPointer<
      QMap<QString, QSharedPointer<GeometricPrimitiveType>>>();
  for (const auto& entry : {GeometricPrimitivePoint::type()})
    (*res)[entry->name()] = entry;
  return res;
}
QSharedPointer<QMap<QString, QSharedPointer<GeometricPrimitiveType>>>
GeometricPrimitive::allTypes() {
  static QSharedPointer<QMap<QString, QSharedPointer<GeometricPrimitiveType>>>
      res = createAllTypes();
  return res;
}

GeometricPrimitivePoint::GeometricPrimitivePoint(const QString& name,
                                                 const QVector3D& position)
    : GeometricPrimitive(name),
      position_(vectorCast<double>(toVector(position))) {}
GeometricPrimitivePoint::GeometricPrimitivePoint(
    const QString& name, const vx::Vector<double, 3>& position)
    : GeometricPrimitive(name), position_(position) {}
GeometricPrimitivePoint::~GeometricPrimitivePoint() {}

QSharedPointer<GeometricPrimitive> GeometricPrimitivePoint::withName(
    const QString& name) {
  return createQSharedPointer<GeometricPrimitivePoint>(name, this->position());
}

QSharedPointer<GeometricPrimitive> GeometricPrimitivePoint::create(
    const QString& name, const QMap<QString, QDBusVariant>& primitiveValues) {
  if (!primitiveValues.contains("Position"))
    throw Exception("de.uni_stuttgart.Voxie.InvalidGeometricPrimitiveValues",
                    "No 'Position' value found for GeometricPrimitivePoint");
  auto position = dbusGetVariantValue<vx::TupleVector<double, 3>>(
      QDBusVariant(primitiveValues["Position"]));
  return createQSharedPointer<GeometricPrimitivePoint>(name,
                                                       toVector(position));
}

QSharedPointer<GeometricPrimitiveType>
GeometricPrimitivePoint::primitiveType() {
  return type();
}

QMap<QString, QDBusVariant> GeometricPrimitivePoint::primitiveValues() {
  QMap<QString, QDBusVariant> result;
  result["Position"] =
      dbusMakeVariant<vx::TupleVector<double, 3>>(toTupleVector(positionVec()));
  return result;
}

QSharedPointer<GeometricPrimitiveType> GeometricPrimitivePoint::type() {
  static QSharedPointer<GeometricPrimitiveType> type =
      makeSharedQObject<GeometricPrimitiveType>(
          "de.uni_stuttgart.Voxie.GeometricPrimitive.Point", "Point",
          QMap<QString, QDBusSignature>{
              {"Position", QDBusSignature("(ddd)")},
          },
          &create);
  return type;
}

QVector3D GeometricPrimitivePoint::position() const {
  return toQVector(vectorCastNarrow<float>(positionVec()));
}
