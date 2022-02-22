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

#include <Voxie/Node/Node.hpp>

#include <Main/Gui/SidePanel.hpp>

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>

// TODO: Rename to SelectNodeConnection.?pp

namespace vx {
class ConnectNodesImpl {
 public:
  virtual ~ConnectNodesImpl();

  virtual void connect() = 0;
  virtual QString description() = 0;
  virtual QPointer<Node> getObj() = 0;
  virtual QSharedPointer<NodeProperty> getProperty() = 0;
  virtual QPointer<Node> getVal() = 0;
};
class ConnectNodesImplInput : public ConnectNodesImpl {
  QPointer<Node> obj;                     // the child
  QSharedPointer<NodeProperty> property;  // property on obj (child)
  QPointer<Node> val;                     // the parent

 public:
  ConnectNodesImplInput(Node* obj, const QSharedPointer<NodeProperty>& property,
                        Node* val);
  void connect() override;
  QString description() override;
  QPointer<Node> getObj() override { return this->obj; }
  QSharedPointer<NodeProperty> getProperty() override { return this->property; }
  QPointer<Node> getVal() override { return this->val; }
};
class ConnectNodesImplOutput : public ConnectNodesImpl {
  QPointer<Node> obj;                     // the parent
  QSharedPointer<NodeProperty> property;  // property on obj (parent)
  QPointer<Node> val;                     // the child

 public:
  ConnectNodesImplOutput(Node* obj,
                         const QSharedPointer<NodeProperty>& property,
                         Node* val);
  void connect() override;
  QString description() override;
  QPointer<Node> getObj() override { return this->obj; }
  QSharedPointer<NodeProperty> getProperty() override { return this->property; }
  QPointer<Node> getVal() override { return this->val; }
};
class ConnectNodesImplInputObject3D : public ConnectNodesImpl {
  QPointer<Node> obj;                     // the child
  QSharedPointer<NodeProperty> property;  // property on obj (child)
  QSharedPointer<NodePrototype>
      prototype;  // the prototype of the new 3D object
  QSharedPointer<NodeProperty> property3D;  // property on the new 3D object
  QPointer<Node> val;                       // the parent

 public:
  ConnectNodesImplInputObject3D(Node* obj,
                                const QSharedPointer<NodeProperty>& property,
                                const QSharedPointer<NodePrototype>& prototype,
                                const QSharedPointer<NodeProperty>& property3D,
                                Node* val);
  void connect() override;
  QString description() override;
  QPointer<Node> getObj() override { return this->obj; }
  QSharedPointer<NodeProperty> getProperty() override { return this->property; }
  QPointer<Node> getVal() override { return this->val; }
};

QList<QSharedPointer<ConnectNodesImpl>> getPossibleConnections(Node* parent,
                                                               Node* child);
bool canBeChildOf(Node* parent, NodePrototype* childPrototype);

class SelectNodeConnection : public QDialog {
  Q_OBJECT

  void finish();

  QListWidget* listView;
  QPushButton* okButton;
  QPushButton* cancelButton;

  QList<QSharedPointer<ConnectNodesImpl>> possibilities;

 public:
  SelectNodeConnection(
      const QList<QSharedPointer<ConnectNodesImpl>>& possibilities);

  static void createNodeConnection(Node* parent, Node* child, int slot);
};
}  // namespace vx
