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

#include <Main/Gui/SelectObjectConnection.hpp>

#include <Voxie/Node/Types.hpp>

#include <Main/Root.hpp>

#include <QAction>
#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

using namespace vx::gui;
using namespace vx;

ConnectNodesImpl::~ConnectNodesImpl() {}

ConnectNodesImplInput::ConnectNodesImplInput(
    Node* obj, const QSharedPointer<NodeProperty>& property, Node* val)
    : obj(obj), property(property), val(val) {}

void ConnectNodesImplInput::connect() {
  if (!obj || !val) {
    qWarning() << "Node disappeared before ConnectNodesImpl::connect()";
    return;
  }

  if (property->type() == types::NodeReferenceType()) {
    obj->setNodeProperty(property, Node::getVariant(val));
  } else if (property->type() == types::NodeReferenceListType()) {
    auto old = obj->getNodeProperty(property);
    QList<Node*> list = Node::parseVariantNodeList(old);
    list.append(val);
    obj->setNodeProperty(property, Node::getVariant(list));
  } else {
    qCritical() << "Unknown reference type: " + property->typeName();
  }
}

QString ConnectNodesImplInput::description() {
  if (property->type() == types::NodeReferenceType())
    return "Set input property " + property->displayName();
  else if (property->type() == types::NodeReferenceListType())
    return "Add to input property " + property->displayName();
  else
    return "Unknown reference type: " + property->typeName();
}

ConnectNodesImplOutput::ConnectNodesImplOutput(
    Node* obj, const QSharedPointer<NodeProperty>& property, Node* val)
    : obj(obj), property(property), val(val) {}

void ConnectNodesImplOutput::connect() {
  if (!obj || !val) {
    qWarning() << "Node disappeared before ConnectNodesImpl::connect()";
    return;
  }
  obj->setNodeProperty(property, Node::getVariant(val));
}

QString ConnectNodesImplOutput::description() {
  return "Set output property " + property->displayName();
}

ConnectNodesImplInputObject3D::ConnectNodesImplInputObject3D(
    Node* obj, const QSharedPointer<NodeProperty>& property,
    const QSharedPointer<NodePrototype>& prototype,
    const QSharedPointer<NodeProperty>& property3D, Node* val)
    : obj(obj),
      property(property),
      prototype(prototype),
      property3D(property3D),
      val(val) {}

void ConnectNodesImplInputObject3D::connect() {
  if (!obj || !val) {
    qWarning() << "Node disappeared before ConnectNodesImpl::connect()";
    return;
  }

  auto obj3D = prototype->create(QMap<QString, QVariant>(), QList<Node*>(),
                                 QMap<QString, QDBusVariant>());

  if (property->type() == types::NodeReferenceType()) {
    obj->setNodeProperty(property, Node::getVariant(obj3D.data()));
  } else if (property->type() == types::NodeReferenceListType()) {
    auto old = obj->getNodeProperty(property);
    QList<Node*> list = Node::parseVariantNodeList(old);
    list.append(obj3D.data());
    obj->setNodeProperty(property, Node::getVariant(list));
  } else {
    qCritical() << "Unknown reference type: " + property->typeName();
  }

  if (property3D->type() == types::NodeReferenceType()) {
    obj3D->setNodeProperty(property3D, Node::getVariant(val));
  } else if (property3D->type() == types::NodeReferenceListType()) {
    auto old = obj3D->getNodeProperty(property3D);
    QList<Node*> list = Node::parseVariantNodeList(old);
    list.append(val);
    obj3D->setNodeProperty(property3D, Node::getVariant(list));
  } else {
    qCritical() << "Unknown reference type: " + property3D->typeName();
  }
}

QString ConnectNodesImplInputObject3D::description() {
  if (property->type() == types::NodeReferenceType())
    return "Create new " + prototype->displayName() + " to set property " +
           property->displayName() + " and set its property " +
           property3D->displayName();
  else if (property->type() == types::NodeReferenceListType())
    return "Create new " + prototype->displayName() + " to add to property " +
           property->displayName() + " and set its property " +
           property3D->displayName();
  else
    return "Unknown reference type: " + property->typeName();
}

SelectNodeConnection::SelectNodeConnection(
    const QList<QSharedPointer<ConnectNodesImpl>>& possibilities)
    : QDialog(), possibilities(possibilities) {
  this->setModal(true);
  this->setWindowTitle("Select connection");
  this->setWindowFlags(Qt::Dialog);

  this->listView = new QListWidget();
  this->listView->setMinimumWidth(500);

  this->okButton = new QPushButton("&Ok");
  this->cancelButton = new QPushButton("&Cancel");

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->addWidget(okButton);
  buttons->addWidget(cancelButton);

  QVBoxLayout* vBox = new QVBoxLayout();
  vBox->addWidget(listView);
  vBox->addLayout(buttons);

  this->setLayout(vBox);

  connect(this->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(this->okButton, &QPushButton::clicked, this,
          &SelectNodeConnection::finish);
  connect(this->listView, &QListWidget::doubleClicked, this,
          &SelectNodeConnection::finish);

  for (const auto& possibility : possibilities) {
    auto newItem = new QListWidgetItem(possibility->description());
    // newItem->setData(Qt::UserRole, QVariant::fromValue(possibility));
    this->listView->addItem(newItem);
  }

  this->show();
}

void SelectNodeConnection::finish() {
  int row = this->listView->currentRow();
  if (row < 0 || row >= this->possibilities.size()) {
    qCritical() << "row out of range";
    this->accept();
    return;
  }
  auto possibility = this->possibilities[row];

  possibility->connect();

  this->accept();
}

QList<QSharedPointer<ConnectNodesImpl>> vx::getPossibleConnections(
    Node* parent, Node* child) {
  QList<QSharedPointer<ConnectNodesImpl>> possibilities;

  // Search for output property in parent which allows child as value
  for (const auto& property : parent->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;

    if (!property->allowsAsValue(child)) continue;

    possibilities << createQSharedPointer<ConnectNodesImplOutput>(
        parent, property, child);
  }

  // Search for input property in child which allows parent as value
  for (const auto& property : child->prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    if (!property->allowsAsValue(parent)) continue;

    possibilities << createQSharedPointer<ConnectNodesImplInput>(
        child, property, parent);
  }

  // Search for input property in child for type Object3D which has a property
  // which allows parent as a value
  for (const auto& property : child->prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    for (const auto& prototype3D : Root::instance()->factories()) {
      if (prototype3D->nodeKind() != NodeKind::Object3D) continue;
      if (!property->allowsAsValue(prototype3D)) continue;

      // Search for input property in prototype3D which allows parent as value
      for (const auto& property3D : prototype3D->nodeProperties()) {
        if (!property3D->isReference() || property3D->isOutputReference())
          continue;

        if (!property3D->allowsAsValue(parent)) continue;

        possibilities << createQSharedPointer<ConnectNodesImplInputObject3D>(
            child, property, prototype3D, property3D, parent);
      }
    }
  }

  return possibilities;
}

// TODO: reduce code duplication with getPossibleConnections()?
bool vx::canBeChildOf(Node* parent, NodePrototype* childPrototype) {
  // Search for output property in parent which allows child as value
  for (const auto& property : parent->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;

    if (!property->allowsAsValue(childPrototype)) continue;

    return true;
  }

  // Search for input property in child which allows parent as value
  for (const auto& property : childPrototype->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    if (!property->allowsAsValue(parent)) continue;

    return true;
  }

  // Search for input property in child for type Object3D which has a property
  // which allows parent as a value
  for (const auto& property : childPrototype->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;

    for (const auto& prototype3D : Root::instance()->factories()) {
      if (prototype3D->nodeKind() != NodeKind::Object3D) continue;
      if (!property->allowsAsValue(prototype3D)) continue;

      // Search for input property in prototype3D which allows parent as value
      for (const auto& property3D : prototype3D->nodeProperties()) {
        if (!property3D->isReference() || property3D->isOutputReference())
          continue;

        if (!property3D->allowsAsValue(parent)) continue;

        return true;
      }
    }
  }

  return false;
}

void SelectNodeConnection::createNodeConnection(Node* parent, Node* child,
                                                int slot) {
  auto possibilities = getPossibleConnections(parent, child);

  if (possibilities.count() == 0) {
    qWarning() << "Unable to add" << parent->prototype() << child
               << "as child of" << child->prototype() << parent;
    return;

  } else if (possibilities.count() == 1) {
    possibilities[0]->connect();

  } else if (slot == -1) {
    qDebug() << "Possibilites for connection" << possibilities.count();
    new SelectNodeConnection(possibilities);
    return;

  } else {  // when connecting directly to InputDot
    possibilities[slot]->connect();
  }
}
