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

#include <Main/Gui/ButtonLabel.hpp>
#include <Main/Gui/GraphWidget.hpp>
#include <QToolButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>

#include <Main/Gui/GraphWidget.hpp>

#include <Voxie/Node/FilterNode.hpp>

namespace vx {
class Root;
class Node;
class FilterNode;
namespace io {
class Operation;
}
namespace gui {
class GraphWidget;

class SidePanel : public QWidget {
  Q_OBJECT

 public:
  explicit SidePanel(Root* root, QMainWindow* mainWindow, QWidget* parent = 0);
  ~SidePanel();
  /**
   * @brief Adds a section to the window.
   * @param widget
   * @param closeable If true, the user can close the section.
   * @param customUi If true, a return button instead of the chevrons is
   * rendered which can switch back to the normal UI
   */
  QWidget* addSection(QWidget* widget, bool closeable = false,
                      vx::Node* obj = nullptr, bool customUi = false);

  void addProgressBar(vx::io::Operation* operation);

  const QList<Node*> selectedNodes() const {
    return dataflowWidget->selectedNodes();
  }

  void setOrientation(Qt::Orientation orientation);

  /**
   * @brief Call this on a node you want to rename. Then a text input box will
   * be displayed where the user can input a new name for the node.
   */
  void userPromptRenameNode(Node* node);

  /**
   * @brief showContextMenu shows the contextmenu for the selected node at the
   * given position
   * @param obj Node which was pressed
   * @param pos position where the context menu should be shown
   */
  void showContextMenu(Node* obj, QPoint& pos);

  bool isPoppedOut() { return sideBarPopout != nullptr; }

  /**
   * @brief dataflowWidget a reference to the dataflow widget
   */
  GraphWidget* dataflowWidget;
  QWidget* dataflowContainer;
  QLineEdit* dataflowRenameBox;

 private:
  void addNode(vx::Node* obj);

  /**
   * @brief SidePanel::performNodeSelection should be called when a node is
   * selected updates some display elements
   */
  void performNodeSelection();

  /**
   * @brief loadCustomUi loads a custom QWidget for the given object into the
   * sidepanel
   * @param obj object containing the custom ui QWidget which is loaded
   */
  void loadCustomUi(vx::Node* obj);

  /**
   * @brief closes the custom Ui sidepanel and emits closed signal
   */
  // TODO: What is 'node' and what means 'nullptr'?
  void closeCustomUi(vx::Node* node = nullptr);

  /**
   * @brief activateVisualizer brings the Visualizer connected to the
   * VisualizerNode
   * @param visualizer VisualizerNode which will be focused
   */
  void activateVisualizer(VisualizerNode* visualizer);

  /**
   * @brief createColorExplanationString Creates a short explanation for a
   * defined Color. Used in the explanation for the GraphWidget.
   * @param color
   * @param explanation
   * @return
   */
  QString createColorExplanationString(QColor color, QString explanation);

  void reorderNodes();

  void onCurrentNodeGroupChanged(NodeGroup* newNodeGroup);

  QVBoxLayout* sections;

  QLayout* bottomLayout;
  QSplitter* splitter;

  QToolBar* toolBar;
  QToolButton* toolButtonRunSelected;
  QToolButton* toolButtonRunAll;
  QAction* toolBarActionDeleteNode;
  QAction* toolBarActionReorder;
  QAction* toolBarActionAutoReorder;
  QAction* toolBarActionExitNodeGroup;
  QAction* toolBarActionExpandSidePanel;
  QAction* toolBarActionPopoutSidePanel;

  QList<QPointer<QWidget>> visibleSections;

  // Sections for which the 'section too large' warning has been emitted
  QSet<QWidget*> sectionTooLargeWarningEmitted;

  // Custom Ui open flag to keep track of sidepanel ui status
  // in case any other node is selected (besides one with a custom Ui) not all
  // other nodes have to be iteratively checked for a open sidepanel status
  bool isCustomUiOpen = false;

  // Current node for context menu
  QPointer<Node> currentNode;

  QMenu* contextMenu;
  QAction* contextMenuActionDataNodeOpen;
  QAction* contextMenuActionDataNode;
  QAction* contextMenuActionFilterNode;
  QAction* contextMenuActionPropertyNode;
  QAction* contextMenuActionVisualizerNode;
  QAction* contextMenuActionObject3DNode;
  QAction* contextMenuActionNodeGroup;
  QAction* contextMenuActionDelete;
  QAction* contextMenuActionRename;
  QAction* contextMenuActionRunFilters;
  QAction* contextMenuActionBringToFront;
  QAction* contextMenuActionDuplicate;
  QAction* contextMenuActionMoveToNodeGroup;
  QAction* contextMenuActionSaveNodeGroup;

  // a simple subclass of QWidget that overrides the close event so that instead
  // of closing the "unpopout" function is executed and the side panel returned
  // to the main window
  class PopoutWidget : public QWidget {
   public:
    PopoutWidget(SidePanel* sidePanel) {
      this->sidePanel = sidePanel;
      resize(1280, 720);
    }

   private:
    SidePanel* sidePanel;

    void closeEvent(QCloseEvent* event) override {
      Q_UNUSED(event);
      sidePanel->toolBarActionPopoutSidePanel->setChecked(false);
    }
  };

  PopoutWidget* sideBarPopout = nullptr;

 Q_SIGNALS:
  void openFile();
  void nodeSelectionChanged();
};

// A widget which exposes the resize event as a signal
class ResizeEventWidget : public QWidget {
  Q_OBJECT

 protected:
  void resizeEvent(QResizeEvent* event) override { Q_EMIT resized(event); }

 Q_SIGNALS:
  void resized(QResizeEvent* event);
};
}  // namespace gui
}  // namespace vx
