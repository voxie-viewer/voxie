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

#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace vx {
class Component;
class ComponentType;

class VOXIEBACKEND_EXPORT ComponentContainer : public vx::RefCountedObject {
  VX_REFCOUNTEDOBJECT

  // Returns componentType->name(), here to avoid having to include
  // ComponentType.hpp
  QString getComponentTypeName(
      const QSharedPointer<ComponentType>& componentType);

 public:
  ComponentContainer(const QString& type);
  ~ComponentContainer();

  virtual QSharedPointer<const QList<QSharedPointer<ComponentType>>>
  componentTypes() = 0;

  QSharedPointer<ComponentType> lookupType(
      const QString& type, bool allowComponentTypeCompatibilityNames);

  virtual QList<QSharedPointer<vx::Component>> listComponents(
      const QSharedPointer<ComponentType>& componentType) = 0;

  // TODO: Move allowMissing=true into separate method?
  virtual QSharedPointer<vx::Component> getComponent(
      const QSharedPointer<ComponentType>& componentType, const QString& name,
      bool allowCompatibilityNames, bool allowMissing = false) = 0;

  template <typename T>
  QList<QSharedPointer<T>> listComponentsTyped() {
    // TODO: Avoid lookup here?
    auto componentType = lookupType(ComponentTypeInfo<T>::name(), false);
    auto list = this->listComponents(componentType);
    QList<QSharedPointer<T>> results;
    for (const auto& component : list) {
      auto componentCast = qSharedPointerDynamicCast<T>(component);
      if (!componentCast)
        throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                            QString() + "Cannot cast to " +
                                getComponentTypeName(componentType));
      results << componentCast;
    }
    return results;
  }

  template <typename T>
  QSharedPointer<T> getComponentTyped(const QString& name,
                                      bool allowCompatibilityNames) {
    // TODO: Avoid lookup here?
    auto componentType = lookupType(ComponentTypeInfo<T>::name(), false);
    auto component =
        this->getComponent(componentType, name, allowCompatibilityNames);
    auto componentCast = qSharedPointerDynamicCast<T>(component);
    if (!componentCast)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InternalError",
          QString() + "Cannot cast to " + getComponentTypeName(componentType));
    return componentCast;
  }

 protected:
  void setContainer(const QSharedPointer<vx::Component>& component);
};

}  // namespace vx
