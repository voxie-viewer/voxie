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

#include "ComponentContainer.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentType.hpp>

using namespace vx;
using namespace vx::plugin;

namespace vx {

class ComponentContainerAdaptorImpl : public ComponentContainerAdaptor {
  ComponentContainer* object;

 public:
  ComponentContainerAdaptorImpl(ComponentContainer* object)
      : ComponentContainerAdaptor(object), object(object) {}
  ~ComponentContainerAdaptorImpl() override {}

  QList<QDBusObjectPath> ListComponents(
      const QString& type,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options,
                                   "AllowComponentTypeCompatibilityNames");

      auto componentType = object->lookupType(
          type, ExportedObject::getOptionValueOrDefault<bool>(
                    options, "AllowComponentTypeCompatibilityNames", true));

      QList<QDBusObjectPath> result;
      for (const auto& res : object->listComponents(componentType))
        result << vx::ExportedObject::getPath(res);
      return result;
    } catch (Exception& e) {
      e.handle(object);
      return QList<QDBusObjectPath>();
    }
  }

  QDBusObjectPath GetComponent(
      const QString& type, const QString& name,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowCompatibilityNames",
                                   "AllowComponentTypeCompatibilityNames");

      auto componentType = object->lookupType(
          type, ExportedObject::getOptionValueOrDefault<bool>(
                    options, "AllowComponentTypeCompatibilityNames", true));

      return vx::ExportedObject::getPath(
          object->getComponent(componentType, name,
                               ExportedObject::getOptionValueOrDefault<bool>(
                                   options, "AllowCompatibilityNames", true)));
    } catch (Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }
};
}  // namespace vx

ComponentContainer::ComponentContainer(const QString& type)
    : RefCountedObject(type) {
  new ComponentContainerAdaptorImpl(this);
}
ComponentContainer::~ComponentContainer() {}

QString ComponentContainer::getComponentTypeName(
    const QSharedPointer<ComponentType>& componentType) {
  return componentType->name();
}

void ComponentContainer::setContainer(
    const QSharedPointer<vx::Component>& component) {
  if (!component)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "ComponentContainer::setContainer(): Got nullptr component");

  component->setContainer(this->thisShared());
}

QSharedPointer<ComponentType> ComponentContainer::lookupType(
    const QString& type, bool allowComponentTypeCompatibilityNames) {
  QSharedPointer<ComponentType> result;
  for (const auto& entry : *this->componentTypes()) {
    bool found = entry->name() == type;
    if (allowComponentTypeCompatibilityNames) {
      for (const auto& compatEntry : entry->compatibilityNames()) {
        auto name = std::get<0>(compatEntry);
        if (name == type) {
          if (std::get<1>(compatEntry))
            qWarning() << "Compatibility component type name" << name
                       << "has been used";
          found = true;
        }
      }
    }
    if (!found) continue;
    if (result)
      throw Exception("de.uni_stuttgart.Voxie.AmbiguousComponentTypeName",
                      "Component type name '" + type + "' is ambiguous");
    result = entry;
  }

  if (!result)
    throw Exception("de.uni_stuttgart.Voxie.InvalidComponentType",
                    "Unknown component type: '" + type + "'");

  return result;
}
