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

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTreeWidget>

#include <Voxie/Node/NodeGroup.hpp>

namespace vx {
namespace gui {

/**
 * @brief A Dialog that can be used to let the user select a node group from all
 * the node groups currently existing in the project or create a new node group.
 */
class NodeGroupSelectWindow : public QDialog {
 public:
  NodeGroupSelectWindow();

  QPointer<NodeGroup> selectedNodeGroup();

 private:
  void addChildGroups(QTreeWidgetItem* parentItem);

  QTreeWidget* twNodeGroups;
  QPushButton* btnNewNodeGroup;
  QDialogButtonBox* buttonBox;

  QPointer<NodeGroup> selectedNodeGroup_ = nullptr;
};

}  // namespace gui
}  // namespace vx
