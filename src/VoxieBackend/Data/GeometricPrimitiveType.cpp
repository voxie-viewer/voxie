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

#include "GeometricPrimitiveType.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;

class GeometricPrimitiveTypeAdaptorImpl : public GeometricPrimitiveTypeAdaptor {
  GeometricPrimitiveType* object;

 public:
  GeometricPrimitiveTypeAdaptorImpl(GeometricPrimitiveType* object)
      : GeometricPrimitiveTypeAdaptor(object), object(object) {}
  ~GeometricPrimitiveTypeAdaptorImpl() override {}

  QString displayName() const override { return object->displayName(); }
  QString name() const override { return object->name(); }
  QMap<QString, QDBusSignature> valueDBusSignatures() const override {
    return object->valueDBusSignatures();
  }
};

GeometricPrimitiveType::GeometricPrimitiveType(
    const QString& name, const QString& displayName,
    const QMap<QString, QDBusSignature>& valueDBusSignatures,
    const CreateFunctionType& createFunction)
    // TODO: Pass json (e.g. for TroveClassifiers)
    : vx::plugin::Component(ComponentTypeInfo<GeometricPrimitiveType>::name(),
                            name, {}),
      displayName_(displayName),
      valueDBusSignatures_(valueDBusSignatures),
      createFunction_(createFunction) {
  new GeometricPrimitiveTypeAdaptorImpl(this);
}
GeometricPrimitiveType::~GeometricPrimitiveType() {}

QList<QString> GeometricPrimitiveType::supportedComponentDBusInterfaces() {
  return {"de.uni_stuttgart.Voxie.GeometricPrimitiveType"};
}
