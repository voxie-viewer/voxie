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

#include <QtWidgets/QWidget>

namespace vx {
class Node;
class HorizontalScrollArea;
namespace gui {
class ButtonLabel;
}

// A section in the side panel showing properties for a node.
class NodeSection : public QWidget {
  Q_OBJECT

  QWidget* contentWidget_;
  HorizontalScrollArea* scrollArea_;
  // TODO: What happens if node_ is destroyed?
  Node* node_;
  bool expanded_;

  vx::gui::ButtonLabel* expandButton;
  void setExpandButtonIcon();

 public:
  // TODO: Is the node parameter needed?
  explicit NodeSection(vx::Node* node, QWidget* contentWidget, bool customUi,
                       bool isInitiallyExpanded);

  QWidget* contentWidget() const { return contentWidget_; }

  bool isExpanded() const { return expanded_; }
  void setExpanded(bool expanded);

 Q_SIGNALS:
  // TODO: What exactly does this do? Clean this up. (This is related to the
  // custom UI of the Segmentation filter.)
  void closeCustomUi(Node* node);
};
}  // namespace vx
