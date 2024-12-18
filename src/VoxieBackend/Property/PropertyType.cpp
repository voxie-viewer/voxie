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

#include "PropertyType.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

using namespace vx;

namespace vx {
namespace {
class PropertyTypeAdaptorImpl : public PropertyTypeAdaptor {
  PropertyType* object;

 public:
  PropertyTypeAdaptorImpl(PropertyType* object)
      : PropertyTypeAdaptor(object), object(object) {}
  ~PropertyTypeAdaptorImpl() override {}

  QString displayName() const override { return object->displayName(); }
  QString name() const override { return object->name(); }
  QDBusVariant defaultValue() const override {
    return object->rawToDBus(object->defaultValue());
  }
  QDBusSignature dBusSignature() const override {
    return object->dbusSignature();
  }
};
}  // namespace
}  // namespace vx

PropertyType::PropertyType(const QString& name, const QString& displayName,
                           const QVariant& defaultValue)
    // TODO: Pass json? (e.g. for TroveClassifiers)
    : Component(ComponentTypeInfo<PropertyType>::name(), name, {}),
      name_(name),
      displayName_(displayName),
      defaultValue_(defaultValue) {
  new PropertyTypeAdaptorImpl(this);
}

PropertyType::~PropertyType() {}

QList<QString> PropertyType::supportedComponentDBusInterfaces() {
  return {"de.uni_stuttgart.Voxie.PropertyType"};
}

PropertyUI* PropertyType::createUISimple(
    const QSharedPointer<PropertyInstance>& propertyInstance) {
  Q_UNUSED(propertyInstance);

  throw vx::Exception(
      "de.uni_stuttgart.Voxie.NotImplemented",
      "Property type " + this->name() + " does not implement createUISimple()");
}
