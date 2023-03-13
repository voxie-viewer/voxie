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

#include "SelectWindow.hpp"

#include <Voxie/Component/HelpCommon.hpp>

#include <Main/Gui/SelectObjectConnection.hpp>

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

#include <Main/Gui/HelpWindow.hpp>

using namespace vx::gui;
using namespace vx;

SelectWindow::SelectWindow(
    QString windowName, SidePanel* sidepanel, Node* currentNode,
    const QList<QSharedPointer<NodePrototype>>& factories, vx::NodeKind kind)
    : QDialog() {
  Q_UNUSED(sidepanel);  // TODO

  this->currentNode = currentNode;
  this->setWindowTitle(windowName);
  this->setWindowFlags(Qt::Dialog);

  this->listView = new QListWidget();
  this->listView->setFixedWidth(270 / 96.0 * this->logicalDpiX());
  this->listView->setMinimumHeight(270 / 96.0 * this->logicalDpiY());
  this->description = new QLabel();
  this->description->setFixedWidth(300 / 96.0 * this->logicalDpiX());
  this->description->setWordWrap(true);
  this->listView->setSortingEnabled(true);

  this->showNonApplicableBox = new QCheckBox("Always show non-&applicable");
  this->showNonApplicableBox->setChecked(true);
  this->showNonStableBox = new QCheckBox("Always show non-&stable");
  // TODO: Set default to false?
  this->showNonStableBox->setChecked(true);
  this->addButton = new QPushButton("&Add");
  this->cancelButton = new QPushButton("&Cancel");
  this->searchBox = new QLineEdit();
  this->searchBox->setPlaceholderText("search...");
  this->searchBox->setClearButtonEnabled(true);
  QAction* searchAction = this->searchBox->addAction(
      QIcon(":/icons/magnifier.png"), QLineEdit::LeadingPosition);

  QHBoxLayout* buttons = new QHBoxLayout();
  buttons->addWidget(addButton);
  buttons->addWidget(cancelButton);

  QVBoxLayout* helpSection = new QVBoxLayout();
  helpSection->addWidget(description);

  this->helpLink = new QLabel();
  voxieRoot().connectLinkHandler(this->helpLink);
  helpSection->addWidget(this->helpLink);

  QHBoxLayout* hBox = new QHBoxLayout();
  hBox->addWidget(listView);
  hBox->addLayout(helpSection);

  QVBoxLayout* vBox = new QVBoxLayout();
  vBox->addWidget(searchBox);
  vBox->addLayout(hBox);
  vBox->addWidget(showNonApplicableBox);
  vBox->addWidget(showNonStableBox);
  vBox->addLayout(buttons);

  this->setLayout(vBox);
  connect(this->showNonApplicableBox, &QCheckBox::toggled, this,
          &SelectWindow::filterNodes);
  connect(this->showNonStableBox, &QCheckBox::toggled, this,
          &SelectWindow::filterNodes);
  connect(this->cancelButton, &QPushButton::released, this, &QDialog::reject);
  connect(this->addButton, &QPushButton::released, this,
          &SelectWindow::addNewNode);
  connect(this->listView, &QListWidget::doubleClicked, this,
          &SelectWindow::addNewNode);
  connect(this->searchBox, &QLineEdit::textChanged, this,
          &SelectWindow::filterNodes);
  connect(searchAction, &QAction::triggered, this, &SelectWindow::filterNodes);

  // Unlink selection window from the initial node if it is destroyed
  if (currentNode) {
    // TODO: Should this be done earlier, when the node state changes?
    connect(currentNode, &QObject::destroyed, this,
            [this]() { this->currentNode = nullptr; });
  }

  qApp->installEventFilter(this);

  QListWidgetItem* newItem;

  for (const auto& prototype : factories) {
    if (prototype->nodeKind() == kind &&
        (!currentNode || canBeChildOf(currentNode, prototype.data()))) {
      newItem = new QListWidgetItem(prototype->displayName());
      newItem->setData(Qt::UserRole, QVariant::fromValue(prototype));
      // if the "Select filter" dialog was opened via the context menu of a data
      // node and tags mismatch
      auto currentDataNode = dynamic_cast<DataNode*>(currentNode);
      bool grayedOut = false;
      if (currentDataNode &&
          !currentDataNode->hasMatchingTags(prototype.data()))
        grayedOut = true;
      // TODO: Gray out / mark non-stable prototypes in some way?
      // if (!prototype->isStable()) grayedOut = true;
      if (grayedOut) {
        newItem->setForeground(this->listView->palette().color(
            QPalette::Disabled, QPalette::Text));
      }
      this->listView->addItem(newItem);
    }
  }

  connect(this->listView, &QListWidget::currentItemChanged, this,
          &SelectWindow::updateDescription);

  // Select the first row to prevent undefined behaviour.
  this->listView->setCurrentRow(0);

  // show the description of the first row
  this->updateDescription();

  this->show();
}

void SelectWindow::filterNodes() {
  QSet<int> shown = getFilteredNodes(this->searchBox->text(),
                                     this->showNonApplicableBox->isChecked(),
                                     this->showNonStableBox->isChecked());
  // If there is no result, try without filters
  // TODO: Show this in the windows somehow?
  if (shown.size() == 0)
    shown = getFilteredNodes(this->searchBox->text(), true, true);

  this->listView->setCurrentItem(nullptr);

  for (int index = this->listView->count() - 1; 0 <= index; index--) {
    auto item = this->listView->item(index);
    if (shown.contains(index)) {
      this->listView->setRowHidden(index, false);
      this->listView->setCurrentItem(item);
    } else {
      this->listView->setRowHidden(index, true);
    }
  }
}

QSet<int> SelectWindow::getFilteredNodes(const QString& searchText,
                                         bool showNonApplicable,
                                         bool showNonStable) {
  QSet<int> res;
  for (int index = this->listView->count() - 1; 0 <= index; index--) {
    auto item = this->listView->item(index);
    QSharedPointer<NodePrototype> prototype =
        item->data(Qt::UserRole).value<QSharedPointer<NodePrototype>>();

    bool show = item->text().contains(searchText, Qt::CaseInsensitive);
    if (show && !showNonStable && !prototype->isStable()) show = false;
    if (show && !showNonApplicable && currentNode) {
      auto dataNode = dynamic_cast<DataNode*>(currentNode);
      if (dataNode && !dataNode->hasMatchingTags(prototype.data()))
        show = false;
    }

    if (show)
      res << index;
  }
  return res;
}

bool SelectWindow::eventFilter(QObject* object, QEvent* event) {
  if (object == searchBox && event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
    if (keyEvent->key() == Qt::Key_Down) {
      listView->setFocus();
    }
  }
  return QObject::eventFilter(object, event);
}

QSharedPointer<NodePrototype> SelectWindow::getSelectedNodePrototype() const {
  if (auto item = this->listView->currentItem()) {
    return qvariant_cast<QSharedPointer<NodePrototype>>(
        item->data(Qt::UserRole));
  } else {
    return QSharedPointer<NodePrototype>();
  }
}

void SelectWindow::updateDescription() {
  if (auto prototype = getSelectedNodePrototype()) {
    this->helpLink->setText(
        "<a href=" + vx::help::uriForPrototype(prototype).toHtmlEscaped() +
        ">View documentation</a>");

    QString inTagString = "";
    QString outTagString = "";
    for (QSharedPointer<NodeProperty> prop : prototype->nodeProperties()) {
      if (!prop->inTags().empty()) {
        if (inTagString.length() == 0) {
          inTagString += "\n\nInput Tags: ";
        } else {
          inTagString += ", ";
        }
        inTagString += NodeTag::joinDisplayNames(prop->inTags(), ", ");
      }
      if (!prop->outTags().empty()) {
        if (outTagString.length() == 0) {
          outTagString += "\n\nOutput Tags: ";
        } else {
          outTagString += ", ";
        }
        outTagString += NodeTag::joinDisplayNames(prop->outTags(), ", ");
      }
    }
    this->description->setText(prototype->description() + inTagString +
                               outTagString);
  } else {
    this->helpLink->setText("");
    this->description->setText("");
  }
}

void SelectWindow::addNewNode() {
  auto item = this->listView->currentItem();
  if (item == nullptr) {
    return;
  }
  try {
    auto nodeprototype =
        qvariant_cast<QSharedPointer<NodePrototype>>(item->data(Qt::UserRole));
    if (!nodeprototype)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "nodeprototype is nullptr");
    auto node = nodeprototype->create(QMap<QString, QVariant>(), QList<Node*>(),
                                      QMap<QString, QDBusVariant>());

    node->setParentNodeGroup(
        Root::instance()
            ->mainWindow()
            ->sidePanel->dataflowWidget->currentNodeGroup());
    if (currentNode) {
      currentNode->addChildNode(node.data());
    }
  } catch (vx::Exception& e) {
    qWarning() << "Failed to create node:" << e.message();
    QMessageBox(QMessageBox::Critical,
                Root::instance()->mainWindow()->windowTitle(),
                QString("Failed to create node: %1").arg(e.message()),
                QMessageBox::Ok, Root::instance()->mainWindow())
        .exec();
  }
  this->accept();
}
