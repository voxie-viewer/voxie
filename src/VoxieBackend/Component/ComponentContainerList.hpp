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

#include <VoxieBackend/Component/ComponentContainer.hpp>

namespace vx {
class ComponentType;

class VOXIEBACKEND_EXPORT ComponentContainerList : public ComponentContainer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ComponentContainerList)

  QSharedPointer<const QList<QSharedPointer<ComponentType>>> componentTypes_;
  QList<QSharedPointer<ComponentContainer>> containers_;

 public:
  ComponentContainerList(
      const QSharedPointer<const QList<QSharedPointer<ComponentType>>>&
          componentTypes,
      const QList<QSharedPointer<ComponentContainer>>& containers);
  ~ComponentContainerList();

  QList<QSharedPointer<vx::Component>> listComponents(
      const QSharedPointer<ComponentType>& componentType) override;

  QSharedPointer<vx::Component> getComponent(
      const QSharedPointer<ComponentType>& componentType, const QString& name,
      bool allowCompatibilityNames) override;

  QSharedPointer<const QList<QSharedPointer<ComponentType>>> componentTypes()
      override {
    return componentTypes_;
  }
};
}  // namespace vx
