/*
 * Copyright (c) 2014-2023 The Voxie Authors
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

#include "NodeNameLineEdit.hpp"

#include <Voxie/DebugOptions.hpp>

#include <Voxie/Node/Node.hpp>

#include <QtGui/QContextMenuEvent>

#include <QtWidgets/QMenu>

vx::NodeNameLineEdit::NodeNameLineEdit(Node* node) : node(node) {
  // this->setPlaceholderText("Placeholder");

  QObject::connect(node, &Node::displayNameChanged, this,
                   &NodeNameLineEdit::updateFromNode);

  QObject::connect(this,
                   (void(QLineEdit::*)(const QString&)) & QLineEdit::textEdited,
                   this, &NodeNameLineEdit::updateFromText);
}

void vx::NodeNameLineEdit::contextMenuEvent(QContextMenuEvent* event) {
  auto mdn = node->manualDisplayName();
  auto isManual = std::get<0>(mdn);

  QMenu* menu = createStandardContextMenu();
  menu->addSeparator();
  auto action = menu->addAction("Reset to automatic name");
  action->setEnabled(isManual);
  QObject::connect(action, &QAction::triggered, this, [this]() {
    if (!this->node) {
      qDebug() << "NodeNameLineEdit::contextMenuEvent callback: Node was "
                  "destroyed";
      return;
    }

    this->node->setManualDisplayName(std::make_tuple(false, ""));
  });
  menu->exec(event->globalPos());
  delete menu;
}

void vx::NodeNameLineEdit::updateFromNode() {
  if (vx::debug_option::Log_NodeNameLineEdit()->enabled())
    qDebug() << "updateFromNode" << suppressUpdate;

  auto mdn = node->manualDisplayName();
  auto isManual = std::get<0>(mdn);
  // auto mname = std::get<1>(mdn);
  if (isManual != isShownManual) {
    if (isManual) {
      this->setStyleSheet("");
    } else {
      this->setStyleSheet("QLineEdit { color: #0000FF }");
    }
    isShownManual = isManual;
  }

  if (!suppressUpdate) {
    auto name = node->displayName();
    if (this->text() != name) {
      this->setText(name);
    }
  }
}

void vx::NodeNameLineEdit::updateFromText(const QString& value) {
  if (vx::debug_option::Log_NodeNameLineEdit()->enabled())
    qDebug() << "updateFromText" << value;

  if (!node) {
    qDebug() << "NodeNameLineEdit::updateFromText(): Node was destroyed";
    return;
  }

  suppressUpdate = true;
  node->setManualDisplayName(std::make_tuple(true, value));
  suppressUpdate = false;
}
