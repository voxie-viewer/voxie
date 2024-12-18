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

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QList>
#include <QMap>
#include <QtCore/QPointer>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusVariant>

class PropertyInstance;

namespace vx {
class Node;
class NodeProperty;

class PropertyUI : public QObject {
  Q_OBJECT

  QSharedPointer<NodeProperty> property_;
  QPointer<Node> node_;
  QSharedPointer<PropertyInstance> propertyInstance_;

  bool initCalled = false;

 protected:
  PropertyUI(const QSharedPointer<NodeProperty>& property, Node* node);
  PropertyUI(const QSharedPointer<PropertyInstance>& propertyInstance);
  ~PropertyUI();

  // Will be called from init()
  virtual void init2();

 public:
  const QSharedPointer<NodeProperty>& property() const { return property_; }
  const QPointer<Node>& node() const { return node_; }
  const QSharedPointer<PropertyInstance>& propertyInstance() const {
    return propertyInstance_;
  }

  virtual bool isMultiline() { return false; }
  virtual QWidget* widget() = 0;

  virtual void setEnabled(bool enabled);

  // Has to be called after the node is fully constructed
  void init();
};

}  // namespace vx
