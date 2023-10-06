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

#include "NodeGroupSelectWindow.hpp"

using namespace vx;
using namespace gui;

NodeGroupSelectWindow::NodeGroupSelectWindow()
    : QDialog(vx::voxieRoot().mainWindow()) {
  setWindowFlags(Qt::Dialog);
  setWindowTitle("Select Node Group");

  setLayout(new QVBoxLayout());

  twNodeGroups = new QTreeWidget();

  // loop through all top-level node groups and add them as top-level items to
  // the tree widget. Then calling addChildGroups() will go through these
  // top-level NodeGroups' child node groups and add them recursively
  for (QSharedPointer<Node> node : voxieRoot().nodes()) {
    NodeGroup* group = dynamic_cast<NodeGroup*>(node.data());
    if (group != nullptr && group->parentNodeGroup() == nullptr) {
      QTreeWidgetItem* item =
          new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr),
                              QStringList(group->displayName()));
      item->setData(0, Qt::UserRole, QVariant::fromValue(group));
      addChildGroups(item);
      twNodeGroups->insertTopLevelItem(0, item);
    }
  }

  // select the first item in the tree
  if (twNodeGroups->topLevelItemCount() > 0) {
    twNodeGroups->setCurrentItem(twNodeGroups->topLevelItem(0));
  }

  twNodeGroups->setHeaderHidden(true);
  layout()->addWidget(twNodeGroups);

  btnNewNodeGroup = new QPushButton("New Node Group");
  connect(btnNewNodeGroup, &QPushButton::pressed, this, [this]() {
    // ask user for name of new node group
    bool dialogResult;
    QString name = QInputDialog::getText(this, "Name of new node group",
                                         "Name:", QLineEdit::Normal, QString(),
                                         &dialogResult);
    if (!dialogResult) return;

    auto node = NodeGroup::getPrototypeSingleton()->create(
        QMap<QString, QVariant>(), QList<Node*>(),
        QMap<QString, QDBusVariant>());
    node->setManualDisplayName(std::make_tuple(true, name));

    // if user has selected a node group in the tree then make new node group a
    // child of that
    if (twNodeGroups->selectedItems().length() > 0) {
      node->setParentNodeGroup(twNodeGroups->selectedItems()[0]
                                   ->data(0, Qt::UserRole)
                                   .value<NodeGroup*>());
    }
    selectedNodeGroup_ = static_cast<NodeGroup*>(node.data());
    accept();
  });
  layout()->addWidget(btnNewNodeGroup);

  buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
    this->selectedNodeGroup_ = this->twNodeGroups->selectedItems().length() == 0
                                   ? nullptr
                                   : this->twNodeGroups->selectedItems()[0]
                                         ->data(0, Qt::UserRole)
                                         .value<NodeGroup*>();
    this->accept();
  });
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout()->addWidget(buttonBox);
}

QPointer<NodeGroup> NodeGroupSelectWindow::selectedNodeGroup() {
  return selectedNodeGroup_;
}

void NodeGroupSelectWindow::addChildGroups(QTreeWidgetItem* parentItem) {
  NodeGroup* parentGroup =
      parentItem->data(0, Qt::UserRole).value<NodeGroup*>();

  for (QSharedPointer<Node> node : parentGroup->groupChildren()) {
    {
      NodeGroup* group = dynamic_cast<NodeGroup*>(node.data());
      if (group != nullptr) {
        QTreeWidgetItem* item =
            new QTreeWidgetItem(parentItem, QStringList(group->displayName()));
        item->setData(0, Qt::UserRole, QVariant::fromValue(group));
        addChildGroups(item);
      }
    }
  }
}
