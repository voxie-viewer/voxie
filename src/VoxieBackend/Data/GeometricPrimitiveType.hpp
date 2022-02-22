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

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

namespace vx {
class GeometricPrimitive;

class VOXIEBACKEND_EXPORT GeometricPrimitiveType
    : public vx::plugin::Component {
  Q_OBJECT

 public:
  using CreateFunctionType = std::function<QSharedPointer<GeometricPrimitive>(
      const QString& name, const QMap<QString, QDBusVariant>& primitiveValues)>;

 private:
  QString displayName_;
  QMap<QString, QDBusSignature> valueDBusSignatures_;
  CreateFunctionType createFunction_;

 public:
  GeometricPrimitiveType(
      const QString& name, const QString& displayName,
      const QMap<QString, QDBusSignature>& valueDBusSignatures,
      const CreateFunctionType& createFunction);
  ~GeometricPrimitiveType();

  static QSharedPointer<GeometricPrimitive> create(
      const QString& primitiveType, const QString& name,
      const QMap<QString, QDBusVariant>& primitiveValues);

  const QString& displayName() const { return displayName_; }
  const QMap<QString, QDBusSignature>& valueDBusSignatures() const {
    return valueDBusSignatures_;
  }
  const CreateFunctionType& createFunction() const { return createFunction_; }

  QList<QString> supportedComponentDBusInterfaces() override;
};

template <>
struct ComponentTypeInfo<GeometricPrimitiveType> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.GeometricPrimitiveType";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.GeometricPrimitiveType", true),
    };
  }
};
}  // namespace vx
