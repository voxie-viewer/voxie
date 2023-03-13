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

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

namespace vx {
class ComponentContainer;

class VOXIEBACKEND_EXPORT Component : public vx::ExportedObject {
  Q_OBJECT

  QString componentType_;
  QString name_;
  QList<QString> troveClassifiers_;
  QSet<QString> troveClassifiersSet_;

  QWeakPointer<vx::ComponentContainer> container_;
  bool containerHasBeenSet_ = false;
  friend class vx::ComponentContainer;  // For calling setContainer()
  void setContainer(const QSharedPointer<vx::ComponentContainer>& container);

 public:
  Component(const QString& componentType, const QJsonObject& json);
  Component(const QString& componentType, const QString& name,
            const QList<QString>& troveClassifiers);
  virtual ~Component();

  const QString& componentType() const { return componentType_; }

  const QString& name() const { return name_; }

  // Set of trove classifiers, see https://pypi.org/project/trove-classifiers/
  const QList<QString>& troveClassifiers() const { return troveClassifiers_; }
  const QSet<QString>& troveClassifiersSet() const {
    return troveClassifiersSet_;
  }

  bool isStable();

  virtual QList<QString> compatibilityNames();

  // TODO: This should inherit from vx::DynamicObject, but then Component would
  // have to be a RefCountedObject
  virtual QList<QString> supportedComponentDBusInterfaces() = 0;

  const QWeakPointer<vx::ComponentContainer>& containerWeak() const {
    return container_;
  }
  // throws if the container is already destroyed or has not been initalized
  QSharedPointer<vx::ComponentContainer> container();
};

namespace plugin {
typedef vx::Component Component;  // TODO: remove
}  // namespace plugin
}  // namespace vx
