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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "Component.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Component/ComponentContainer.hpp>

#include <VoxieClient/JsonUtil.hpp>

using namespace vx::plugin;

namespace vx {
class ComponentAdaptorImpl : public ComponentAdaptor {
  Component* object;

 public:
  ComponentAdaptorImpl(Component* object)
      : ComponentAdaptor(object), object(object) {}
  ~ComponentAdaptorImpl() override {}

  QString componentType() const override {
    try {
      return object->componentType();
    } catch (vx::Exception& e) {
      e.handle(object);
      return "";
    }
  }

  QString name() const override {
    try {
      return object->name();
    } catch (vx::Exception& e) {
      e.handle(object);
      return "";
    }
  }

  QDBusObjectPath componentContainer() const override {
    try {
      return vx::ExportedObject::getPath(object->container());
    } catch (vx::Exception& e) {
      e.handle(object);
      return vx::ExportedObject::getPath(nullptr);
    }
  }
};

class DynamicObjectAdaptorImplComponent : public DynamicObjectAdaptor {
  Component* object;

 public:
  DynamicObjectAdaptorImplComponent(Component* object)
      : DynamicObjectAdaptor(object), object(object) {}
  ~DynamicObjectAdaptorImplComponent() override {}

  QStringList supportedInterfaces() const override {
    try {
      return object->supportedComponentDBusInterfaces();
    } catch (Exception& e) {
      e.handle(object);
      return {};
    }
  }
};
}  // namespace vx

static QString getShortType(const QString& componentType) {
  int index = componentType.lastIndexOf('.');
  if (index == -1)
    return componentType;
  else
    return componentType.mid(index + 1);
}

Component::Component(const QString& componentType, const QString& name,
                     const QList<QString>& troveClassifiers)
    : ExportedObject("Component/" + getShortType(componentType)),
      componentType_(componentType),
      name_(name),
      troveClassifiers_(troveClassifiers),
      container_() {
  new ComponentAdaptorImpl(this);
  new DynamicObjectAdaptorImplComponent(this);

  for (const auto& classifier : troveClassifiers_)
    troveClassifiersSet_.insert(classifier);
}
Component::Component(const QString& componentType, const QJsonObject& json)
    : Component(componentType, expectString(json["Name"]),
                json.contains("TroveClassifiers")
                    ? expectStringList(json["TroveClassifiers"])
                    : QList<QString>()) {}
Component::~Component() {}

void Component::setContainer(
    const QSharedPointer<vx::ComponentContainer>& container) {
  if (!container)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Component::setContainer(): Got nullptr container");

  if (this->containerHasBeenSet_)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Component::setContainer(): container is already set");

  this->container_ = container;
  this->containerHasBeenSet_ = true;
}

QSharedPointer<vx::ComponentContainer> Component::container() {
  auto ext = containerWeak().lock();
  if (!ext) {
    // TODO: how should this be handled?
    if (containerHasBeenSet_)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Container object is already destroyed");
  }
  return ext;
}

bool Component::isStable() {
  return this->troveClassifiersSet().contains(
             "Development Status :: 5 - Production/Stable") ||
         this->troveClassifiersSet().contains(
             "Development Status :: 6 - Mature");
}

// TODO: Implement compatibilityNames for components other than NodePrototype
// and PropertyType?
QList<QString> Component::compatibilityNames() { return QList<QString>(); }
