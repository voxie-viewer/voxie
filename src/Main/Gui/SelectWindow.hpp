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

#include <Main/Gui/SidePanel.hpp>
#include <Voxie/Node/Node.hpp>

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>

namespace vx {
namespace gui {
class SelectWindow : public QDialog {
 public:
  SelectWindow(QString windowName, SidePanel* sidepanel, vx::Node* currentNode,
               const QList<QSharedPointer<vx::NodePrototype>>& factories,
               vx::NodeKind kind);

  vx::Node* currentNode;
  QListWidget* listView;
  QLabel* description;
  QLabel* helpLink;
  QPushButton* addButton;
  QPushButton* cancelButton;
  QLineEdit* searchBox;
  QCheckBox* checkBox;

  /**
   * @brief Call this to add the currently selected item as child to its parent
   * (currentNode).
   */
  virtual void addNewNode();

  /**
   * @brief filterNodes(): Filters the displayed nodes in the listView on
   * the basis of the entered text in searchBox.
   *
   * Only if the displayed name of a node contains the entered text in the
   * searchBox, the node is shown. The nodes are filtered whenever the text
   * in the searchBox is changed and the magnifier icon is cicked.
   */
  void filterNodes();

  void toggleHiddenState(bool checked);

 protected:
  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  QSharedPointer<NodePrototype> getSelectedNodePrototype() const;

  void updateDescription();
};
}  // namespace gui
}  // namespace vx
