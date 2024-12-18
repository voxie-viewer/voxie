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

#include "ComponentContainerList.hpp"

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentType.hpp>

using namespace vx;

ComponentContainerList::ComponentContainerList(
    const QSharedPointer<const QList<QSharedPointer<ComponentType>>>&
        componentTypes,
    const QList<QSharedPointer<ComponentContainer>>& containers)
    : ComponentContainer("ComponentContainerList"),
      componentTypes_(componentTypes),
      containers_(containers) {}
ComponentContainerList::~ComponentContainerList() {}

QList<QSharedPointer<vx::Component>> ComponentContainerList::listComponents(
    const QSharedPointer<ComponentType>& componentType) {
  QList<QSharedPointer<vx::Component>> result;
  for (const auto& container : containers_) {
    result << container->listComponents(componentType);
  }
  return result;
}

QSharedPointer<vx::Component> ComponentContainerList::getComponent(
    const QSharedPointer<ComponentType>& componentType, const QString& name,
    bool allowCompatibilityNames, bool allowMissing) {
  QSharedPointer<Component> result;

  for (const auto& container : containers_) {
    for (const auto& component : container->listComponents(componentType)) {
      bool found = component->name() == name;
      if (allowCompatibilityNames) {
        for (const auto& n : component->compatibilityNames()) {
          if (n == name) found = true;
        }
      }
      if (!found) continue;
      if (result)
        throw Exception("de.uni_stuttgart.Voxie.AmbiguousComponentName",
                        "Component name '" + name + "' for type '" +
                            componentType->name() + "' is ambiguous");
      result = component;
    }
  }
  if (!result && !allowMissing)
    throw Exception("de.uni_stuttgart.Voxie.ComponentNotFound",
                    "Could not find component '" + name + "' with type '" +
                        componentType->name() + "'");

  return result;
}
