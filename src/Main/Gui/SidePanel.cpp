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

#include "SidePanel.hpp"

#include <Main/Root.hpp>

#include <Main/Component/SessionManager.hpp>
#include <Main/Gui/ButtonLabel.hpp>
#include <Main/Gui/NodeGroupSelectWindow.hpp>
#include <Main/Gui/SelectWindow.hpp>

#include <Voxie/Component/Tool.hpp>
#include <VoxieBackend/Component/Extension.hpp>

#include <Main/IO/RunAllFilterOperation.hpp>
#include <Main/IO/RunMultipleFilterOperation.hpp>
#include <VoxieBackend/IO/Operation.hpp>

#include <Voxie/Component/Plugin.hpp>
#include <Voxie/IO/SaveFileDialog.hpp>
#include <Voxie/Node/NodeGroup.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>

#include <VoxieClient/Exception.hpp>

#include <QtCore/QPointer>
#include <QtCore/QStack>

#include <QVariant>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QToolBar>

using namespace vx;
using namespace vx::gui;
using namespace vx::io;

SidePanel::SidePanel(vx::Root* root, QMainWindow* mainWindow,
                     QWidget* parentWidget)
    : QWidget(parentWidget) {
  // this->setMinimumWidth(405);
  this->setMinimumWidth(405 / 96.0 * this->logicalDpiX());

  auto layout = new QVBoxLayout();
  layout->setMargin(0);
  this->setLayout(layout);

  toolBar = new QToolBar();
  layout->addWidget(toolBar);

  splitter = new QSplitter(Qt::Vertical);
  layout->addWidget(splitter);

  dataflowContainer = new QWidget();
  {
    // build dataflow section with will contain graph and legend
    QVBoxLayout* dataflowLayout = new QVBoxLayout();
    dataflowContainer->setLayout(dataflowLayout);
    dataflowLayout->setSpacing(0);
    dataflowLayout->setMargin(0);

    dataflowWidget = new GraphWidget(root, this);

    // toolbar
    // remove gigantic margins around the tool bar items
    toolBar->setContentsMargins(0, 0, 0, 0);
    toolBar->setStyleSheet("QToolBar { padding: 0; }");

    toolButtonRunSelected = new QToolButton();
    toolBar->addWidget(toolButtonRunSelected);
    toolButtonRunSelected->setPopupMode(QToolButton::MenuButtonPopup);
    QMenu* runSelectedMenu = new QMenu();
    QAction* actionRunSelected =
        runSelectedMenu->addAction(QIcon(":/icons/control.png"), nullptr);
    actionRunSelected->setToolTip("Run selected filter(s) if changed");
    actionRunSelected->setText("Run selected filter(s) if changed");
    connect(actionRunSelected, &QAction::triggered, this, [this] {
      if (dataflowWidget->selectedNodes().length() == 1) {
        auto selectedFilter =
            dynamic_cast<vx::FilterNode*>(dataflowWidget->selectedNodes()[0]);

        if (selectedFilter) {
          if (selectedFilter->needsRecalculation()) selectedFilter->run();
        } else {
          qWarning() << "Tried to run selected node as a filter even though"
                        " selected node is not of type FilterNode.";
        }
      } else if (dataflowWidget->selectedNodes().length() > 1) {
        auto op = RunMultipleFilterOperation::create();

        QList<vx::FilterNode*> filterList;
        for (auto obj : dataflowWidget->selectedNodes()) {
          if (obj->nodeKind() == NodeKind::Filter)
            filterList.append(dynamic_cast<vx::FilterNode*>(obj));
        }

        op->runMultiple(filterList, true);
      }
    });
    QAction* actionForceRunSelected = runSelectedMenu->addAction(
        QIcon(":/icons-voxie/control-red.png"), nullptr);
    actionForceRunSelected->setToolTip("Force run selected filter(s)");
    actionForceRunSelected->setText("Force run selected filter(s)");
    connect(actionForceRunSelected, &QAction::triggered, this, [this] {
      if (dataflowWidget->selectedNodes().length() == 1) {
        auto selectedFilter =
            dynamic_cast<vx::FilterNode*>(dataflowWidget->selectedNodes()[0]);

        if (selectedFilter) {
          selectedFilter->run();
        } else {
          qWarning() << "Tried to run selected node as a filter even though"
                        " selected node is not of type FilterNode.";
        }
      } else if (dataflowWidget->selectedNodes().length() > 1) {
        auto op = RunMultipleFilterOperation::create();

        QList<vx::FilterNode*> filterList;
        for (auto obj : dataflowWidget->selectedNodes()) {
          if (obj->nodeKind() == NodeKind::Filter)
            filterList.append(dynamic_cast<vx::FilterNode*>(obj));
        }

        op->runMultiple(filterList, false);
      }
    });

    QAction* actionRunSelectedAndParents = runSelectedMenu->addAction(
        QIcon(":/icons-voxie/control-from-previous.png"), nullptr);
    actionRunSelectedAndParents->setToolTip(
        "Run the selected filter(s) and their parents");
    actionRunSelectedAndParents->setText(
        "Run the selected filter(s) and their parents");
    connect(actionRunSelectedAndParents, &QAction::triggered, this, [this] {
      auto op = RunMultipleFilterOperation::create();

      QSet<vx::FilterNode*> filterSet;

      // depth-first search to get all parent filter nodes
      for (auto selectedObj : dataflowWidget->selectedNodes()) {
        QStack<vx::Node*> stack;
        stack.push(selectedObj);

        while (!stack.isEmpty()) {
          auto obj = stack.pop();

          if (obj->nodeKind() == NodeKind::Filter)
            filterSet.insert(dynamic_cast<FilterNode*>(obj));

          for (auto parent : obj->parentNodes()) {
            if (parent->nodeKind() == NodeKind::Filter) {
              stack.push(parent);
            }
          }
        }
      }

      op->runMultiple(filterSet.values(), true);
    });

    QAction* actionRunSelectedAndChildren = runSelectedMenu->addAction(
        QIcon(":/icons-voxie/control-to-next.png"), nullptr);
    actionRunSelectedAndChildren->setToolTip(
        "Run the selected filter(s) and all their children");
    actionRunSelectedAndChildren->setText(
        "Run the selected filter(s) and all their children");
    connect(actionRunSelectedAndChildren, &QAction::triggered, this, [this] {
      auto op = RunMultipleFilterOperation::create();

      QSet<vx::FilterNode*> filterSet;

      // depth-first search to get all parent filter nodes
      for (auto selectedObj : dataflowWidget->selectedNodes()) {
        QStack<vx::Node*> stack;
        stack.push(selectedObj);

        while (!stack.isEmpty()) {
          auto obj = stack.pop();

          if (obj->nodeKind() == NodeKind::Filter)
            filterSet.insert(dynamic_cast<FilterNode*>(obj));

          for (auto parent : obj->childNodes()) {
            if (parent->nodeKind() == NodeKind::Filter) {
              stack.push(parent);
            }
          }
        }
      }

      op->runMultiple(filterSet.values(), true);
    });
    toolButtonRunSelected->setMenu(runSelectedMenu);
    toolButtonRunSelected->setDefaultAction(actionRunSelected);

    // when a different node is selected check that exactly one node is
    // selected and that it is of type FilterNode (so when can actually
    // run it). If not disable the "run selected" button
    connect(dataflowWidget, &GraphWidget::selectionChanged, this,
            [this](QList<Node*> selectedNodes) {
              this->toolButtonRunSelected->setEnabled(false);

              if (selectedNodes.length() == 1 &&
                  selectedNodes[0]->nodeKind() == NodeKind::Filter)
                this->toolButtonRunSelected->setEnabled(true);
            });

    toolButtonRunAll = new QToolButton();
    toolButtonRunAll->setPopupMode(QToolButton::MenuButtonPopup);
    QMenu* runAllMenu = new QMenu();

    QAction* actionRunAllChanged =
        runAllMenu->addAction(QIcon(":/icons/control-skip.png"), nullptr);
    actionRunAllChanged->setToolTip("Run all changed filters");
    actionRunAllChanged->setText("Run all changed filters");
    connect(actionRunAllChanged, &QAction::triggered, this,
            [this] { RunAllFilterOperation::create()->runAll(true); });
    toolButtonRunAll->setMenu(runAllMenu);
    toolButtonRunAll->setDefaultAction(actionRunAllChanged);

    QAction* actionRunAll = runAllMenu->addAction(
        QIcon(":/icons-voxie/control-skip-red.png"), nullptr);
    actionRunAll->setToolTip("Force-run all filters");
    actionRunAll->setText("Force-run all filters");
    connect(actionRunAll, &QAction::triggered, this,
            [this] { RunAllFilterOperation::create()->runAll(); });

    toolBar->addWidget(toolButtonRunAll);

    toolBar->addSeparator();

    toolBarActionReorder =
        toolBar->addAction(QIcon(":/icons/arrow-turn-000-left.png"), nullptr);
    toolBarActionReorder->setToolTip("Reorder nodes");
    connect(toolBarActionReorder, &QAction::triggered, this,
            &SidePanel::reorderNodes);

    toolBarActionAutoReorder = toolBar->addAction("Auto Reorder");
    toolBarActionAutoReorder->setCheckable(true);
    toolBarActionAutoReorder->setToolTip(
        "Enable or disable node auto reordering");
    connect(toolBarActionAutoReorder, &QAction::toggled, this,
            [this](bool checked) { dataflowWidget->setAutoReorder(checked); });

    toolBar->addSeparator();

    toolBarActionDeleteNode =
        toolBar->addAction(QIcon(":/icons/minus.png"), nullptr);
    toolBarActionDeleteNode->setToolTip("Delete selected node");
    connect(toolBarActionDeleteNode, &QAction::triggered, this,
            [=]() { dataflowWidget->deleteSelectedNodes(); });

    // add spacer so that following button is right-aligned
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(spacer);

    toolBarActionExitNodeGroup =
        toolBar->addAction(QIcon(":/icons/cross-button.png"), nullptr);
    connect(toolBarActionExitNodeGroup, &QAction::triggered, this, [this] {
      dataflowWidget->setCurrentNodeGroup(
          dataflowWidget->currentNodeGroup()->parentNodeGroup());
    });

    toolBarActionPopoutSidePanel =
        toolBar->addAction(QIcon(":/icons/applications.png"), nullptr);
    toolBarActionPopoutSidePanel->setCheckable(true);
    toolBarActionPopoutSidePanel->setToolTip(
        "Popout the side panel to a separate window");
    connect(
        toolBarActionPopoutSidePanel, &QAction::toggled, this,
        [this](bool checked) {
          if (checked) {
            // construct a simple popout container to hold the side bar
            sideBarPopout = new PopoutWidget(this);
            sideBarPopout->setWindowTitle("Voxie Node Graph");
            sideBarPopout->setLayout(new QGridLayout());
            sideBarPopout->layout()->addWidget(this);
            toolBarActionExpandSidePanel->setChecked(true);
            sideBarPopout->show();

            // make sure the mdi area is visible if the side panel is popped out
            Root::instance()->mainWindow()->getMdiArea()->setFixedSize(
                QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
          } else {
            if (sideBarPopout != nullptr) {
              if (Root::instance()->mainWindow()->isSidePanelOnLeft()) {
                ((QSplitter*)Root::instance()->mainWindow()->centralWidget())
                    ->insertWidget(0, this);
              } else {
                ((QSplitter*)Root::instance()->mainWindow()->centralWidget())
                    ->addWidget(this);
              }
              toolBarActionExpandSidePanel->setChecked(false);
              sideBarPopout->close();
              sideBarPopout = nullptr;
            }
          }
        });

    toolBarActionExpandSidePanel =
        toolBar->addAction(QIcon(":/icons-voxie/skip-next.png"), nullptr);
    toolBarActionExpandSidePanel->setCheckable(true);
    toolBarActionExpandSidePanel->setToolTip(
        "Expand the graph view to the whole window");
    connect(toolBarActionExpandSidePanel, &QAction::toggled, this,
            [this](bool checked) {
              if (checked) {
                // only hide the mdi area if the side bar is not popped out
                if (!this->isPoppedOut()) {
                  // we can't use hide() and show() to hide/show the mdi area
                  // because of graphical glitches, so instead just make it very
                  // small
                  Root::instance()->mainWindow()->getMdiArea()->setFixedSize(0,
                                                                             0);
                }

                this->setOrientation(Qt::Horizontal);
                this->splitter->setSizes({100, 150});
                this->toolBarActionExpandSidePanel->setIcon(
                    QIcon(":/icons-voxie/skip-previous.png"));
              } else {
                // we can't use hide() and show() to hide/show the mdi area
                // because of graphical glitches, reset small size to normal
                // size
                Root::instance()->mainWindow()->getMdiArea()->setFixedSize(
                    QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
                this->setOrientation(Qt::Vertical);
                this->splitter->setSizes({1000, 3000});
                this->toolBarActionExpandSidePanel->setIcon(
                    QIcon(":/icons-voxie/skip-next.png"));
              }
              // TODO: Is this really needed?
              Root::instance()->mainWindow()->update();
            });
    // graph widget
    dataflowLayout->addWidget(dataflowWidget);

    // node rename box
    this->dataflowRenameBox = new QLineEdit(this);
    this->dataflowRenameBox->setVisible(false);
    dataflowLayout->addWidget(this->dataflowRenameBox);

    // legend
    dataflowLayout->addWidget(new QLabel(
        this->createColorExplanationString(this->dataflowWidget->dataNodeColor,
                                           "Data") +
        " " +
        this->createColorExplanationString(
            this->dataflowWidget->filterNodeColor, "Filter") +
        " " +
        this->createColorExplanationString(
            this->dataflowWidget->propertyNodeColor, "Property") +
        " " +
        this->createColorExplanationString(
            this->dataflowWidget->visualizerNodeColor, "Visualizer") +
        " " +
        this->createColorExplanationString(
            this->dataflowWidget->object3DNodeColor, "3D Object")));
  }

  splitter->addWidget(dataflowContainer);
  splitter->setCollapsible(0, false);

  auto bottomWidget = new QWidget();
  splitter->addWidget(bottomWidget);
  splitter->setCollapsible(1, false);
  bottomLayout = new QVBoxLayout();
  bottomLayout->setMargin(0);
  bottomWidget->setLayout(bottomLayout);

  auto scroll = new QScrollArea();
  bottomLayout->addWidget(scroll);
  scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroll->setWidgetResizable(true);
  {
    auto boxC = new ResizeEventWidget();
    QVBoxLayout* box = new QVBoxLayout(boxC);
    box->setMargin(0);
    box->setSpacing(2);
    {
      this->sections = new QVBoxLayout();
      this->sections->setMargin(3);
      box->addLayout(this->sections);
      box->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
                                         QSizePolicy::Expanding));
    }
    scroll->setWidget(boxC);
    QObject::connect(
        boxC, &ResizeEventWidget::resized, this, [this](QResizeEvent* event) {
          // This code will check whether a sidepanel section is too large and
          // print a warning in this case. This is done in the resize event
          // because then we can be sure that the section minimumSizeHint is up
          // to date and if the size of the section changes later on this also
          // will be caught. In order to avoid warning spam the message will be
          // shown only once for each section.
          Q_UNUSED(event);
          // qDebug() << "resize" << event << visibleSections.size();
          for (auto section : visibleSections) {
            if (!section) continue;
            auto maxAllowed = 425 / 96.0 * section->logicalDpiX();
            auto minSize = section->minimumSizeHint();
            auto minWidth = minSize.width();
            if (minWidth <= maxAllowed) continue;
            /*
            qDebug() << "sizecheck" << section->windowTitle() << minWidth
                     << maxAllowed
                     << sectionTooLargeWarningEmitted.contains(section);
            */
            if (sectionTooLargeWarningEmitted.contains(section)) continue;
            qWarning() << "Warning: Section" << section->windowTitle()
                       << "requires a width of" << minWidth
                       << "px instead of the allowed" << maxAllowed;
            sectionTooLargeWarningEmitted.insert(section);
          }
        });
  }

  splitter->setSizes({1000, 3000});

  connect(root, &vx::Root::nodeAdded, this, &SidePanel::addNode);
  connect(dataflowWidget, &GraphWidget::selectionChanged, this, [this]() {
    performNodeSelection();
    Q_EMIT nodeSelectionChanged();
  });

  connect(dataflowWidget, &GraphWidget::nodeActivated, this, [this](Node* obj) {
    auto vis = qobject_cast<VisualizerNode*>(obj);
    activateVisualizer(vis);
  });

  connect(dataflowWidget, &GraphWidget::nodeActivated, this, [this](Node* obj) {
    auto vis = qobject_cast<VisualizerNode*>(obj);
    activateVisualizer(vis);
  });

  this->contextMenu = new QMenu(mainWindow);

  this->contextMenuActionBringToFront = contextMenu->addAction(
      QIcon(":/icons/arrow-stop-090.png"), "Bring to &Front");
  connect(this->contextMenuActionBringToFront, &QAction::triggered, this,
          [this] {
            auto vis = qobject_cast<VisualizerNode*>(currentNode);
            activateVisualizer(vis);
          });

  this->contextMenuActionDataNodeOpen =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "&Open file");
  connect(this->contextMenuActionDataNodeOpen, &QAction::triggered, this,
          [this] { Q_EMIT this->openFile(); });

  this->contextMenuActionDataNode =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "New empty &data node");
  connect(this->contextMenuActionDataNode, &QAction::triggered, this, [this] {
    new SelectWindow("Select data node", this, qobject_cast<Node*>(currentNode),
                     Root::instance()->factories(), NodeKind::Data);
  });

  this->contextMenuActionFilterNode =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "New &filter node");
  connect(this->contextMenuActionFilterNode, &QAction::triggered, this, [this] {
    new SelectWindow("Select Filter", this, qobject_cast<Node*>(currentNode),
                     Root::instance()->factories(), NodeKind::Filter);
  });

  this->contextMenuActionPropertyNode =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "New &property node");
  connect(this->contextMenuActionPropertyNode, &QAction::triggered, this,
          [this] {
            new SelectWindow("Select Property", this, currentNode.data(),
                             Root::instance()->factories(), NodeKind::Property);
          });

  this->contextMenuActionVisualizerNode =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "New &visualizer node");
  connect(
      this->contextMenuActionVisualizerNode, &QAction::triggered, this, [this] {
        new SelectWindow("Select Visualizer", this, currentNode.data(),
                         Root::instance()->factories(), NodeKind::Visualizer);
      });

  this->contextMenuActionObject3DNode =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "New &3D object node");
  connect(this->contextMenuActionObject3DNode, &QAction::triggered, this,
          [this] {
            new SelectWindow("Select 3D Object", this, currentNode.data(),
                             Root::instance()->factories(), NodeKind::Object3D);
          });

  this->contextMenuActionNodeGroup =
      contextMenu->addAction(QIcon(":/icons/plus.png"), "New node &group");
  connect(this->contextMenuActionNodeGroup, &QAction::triggered, this, [this] {
    auto node = NodeGroup::getPrototypeSingleton()->create(
        QMap<QString, QVariant>(), QList<Node*>(),
        QMap<QString, QDBusVariant>());
    node->setParentNodeGroup(dataflowWidget->currentNodeGroup());
  });

  this->contextMenuActionDelete =
      contextMenu->addAction(QIcon(":/icons/minus.png"), "Delete");
  connect(this->contextMenuActionDelete, &QAction::triggered, this,
          [this] { dataflowWidget->deleteSelectedNodes(); });

  this->contextMenuActionRename =
      contextMenu->addAction(QIcon(":/icons/pencil.png"), "Rename");
  connect(contextMenuActionRename, &QAction::triggered, this, [this] {
    // can only rename one node at a time
    if (selectedNodes().length() != 1) return;

    userPromptRenameNode(selectedNodes()[0]);
  });

  contextMenuActionDuplicate =
      contextMenu->addAction(QIcon(":/icons/document-copy.png"), "Duplicate");
  connect(contextMenuActionDuplicate, &QAction::triggered, this, [this] {
    QMap<Node*, QSharedPointer<Node>> newNodes =
        QMap<Node*, QSharedPointer<Node>>();

    QList<Node*> oldSelectedNodes = selectedNodes();
    QList<Node*> newSelectedNodes;
    for (Node* node : oldSelectedNodes) {
      QSharedPointer<Node> newNode = node->prototype()->create(
          QVariantMap(), QList<Node*>(), QMap<QString, QDBusVariant>());
      newNodes.insert(node, newNode);
      voxieRoot().setGraphPosition(
          newNode.data(),
          voxieRoot().getGraphPosition(node) + QVector2D(150, 0));
      newSelectedNodes << newNode.data();
    }
    dataflowWidget->setSelectedNodes(newSelectedNodes);

    for (Node* node : oldSelectedNodes) {
      // cloning of data nodes
      DataNode* oldDataNode = dynamic_cast<DataNode*>(node);
      if (oldDataNode) {
        DataNode* newDataNode = dynamic_cast<DataNode*>(newNodes[node].data());

        if (oldDataNode->importer() != nullptr) {
          // this is a data node that contains data from an external source
          // (e.g. file). As this data cannot be modified in Voxie anyway we can
          // just do a shallow copy and not clone the data. Instead let the
          // cloned node reference the same data as the original node.
          newDataNode->setData(oldDataNode->data());
          newDataNode->setFileInfo(oldDataNode->getFileInfo(),
                                   oldDataNode->importer(),
                                   oldDataNode->importProperties());
          newDataNode->setAutomaticDisplayName(
              newDataNode->getFileInfo().fileName());
        } else {
          // TODO: if it is an "internal" data node (whose data can change) make
          // a deep copy of the whole data object
        }
      }

      // copy over properties
      auto properties = QVariantMap();
      for (QSharedPointer<NodeProperty> property :
           node->prototype()->nodeProperties()) {
        // if property is a connection to another node, check that that node is
        // also selected. Otherwise don't copy the connection
        if (property->isReference()) {
          QDBusObjectPath objPath =
              node->getNodeProperty(property).value<QDBusObjectPath>();
          for (Node* otherNode : oldSelectedNodes) {
            if (otherNode->getPath() == objPath) {
              properties.insert(
                  property->name(),
                  QVariant::fromValue(newNodes[otherNode]->getPath()));
              break;
            }
          }
        } else {
          properties.insert(property->name(), node->getNodeProperty(property));
        }
      }

      /*for (auto key : node->propertyValues().keys()) {
        auto variant = node->propertyValues().value(key);
        qDebug() << variant.typeName();

        // if property is a connection to another node, check that that node is
        // also selected. Otherwise don't copy the connection
        if (variant.canConvert<QDBusObjectPath>()) {
          QDBusObjectPath objectPath = variant.value<QDBusObjectPath>();
          for (Node* otherNode : oldSelectedNodes) {
            if (otherNode->getPath() == objectPath) {
              properties.insert(key, QVariant::fromValue(
                                         newNodes.value(otherNode)->getPath()));
              break;
            }
          }
        } else {
          properties.insert(key, variant);
        }
      }*/

      newNodes[node]->setNodeProperties(properties);
    }
  });

  contextMenuActionMoveToNodeGroup = contextMenu->addAction(
      QIcon(":/icons/document-import.png"), "Move to node group");
  connect(contextMenuActionMoveToNodeGroup, &QAction::triggered, this, [this] {
    NodeGroupSelectWindow selectWindow;

    if (selectWindow.exec() != QDialog::Accepted) return;

    NodeGroup* selectedGroup = selectWindow.selectedNodeGroup();

    if (selectedGroup != nullptr) {
      // check to make sure that user isn't trying to move a node group into
      // itself
      if (selectedNodes().contains(selectedGroup)) {
        QMessageBox msgBox(this);
        msgBox.setText("Cannot move a node group into itself!");
        msgBox.exec();
        return;
      }

      for (Node* node : selectedNodes()) {
        // disconnect edges between nodes that are now in the node group and
        // nodes that are outside
        for (Node* parentNode : node->parentNodes()) {
          if (selectedNodes().contains(parentNode)) continue;
          if (parentNode->parentNodeGroup() != selectedGroup) {
            parentNode->removeChildNode(node);
          }
        }

        for (Node* childNode : node->childNodes()) {
          if (selectedNodes().contains(childNode)) continue;
          if (childNode->parentNodeGroup() != selectedGroup) {
            node->removeChildNode(childNode);
          }
        }
      }

      for (Node* node : selectedNodes()) {
        // move the nodes to the node group
        node->setParentNodeGroup(selectedGroup);
      }
    }
  });

  contextMenuActionSaveNodeGroup = contextMenu->addAction(
      QIcon(":/icons/disk-black.png"), "Save node group");
  connect(contextMenuActionSaveNodeGroup, &QAction::triggered, this, [this] {
    SessionManager* sessionManager = new SessionManager();

    vx::io::SaveFileDialog dialog(this, "Save node group", QString());
    dialog.addFilter(FilenameFilter("Voxie Node Group", {"*.ngrp.py"}),
                     nullptr);
    dialog.setup();

    // TODO: This should be asynchronous
    if (dialog.exec() != QDialog::Accepted) return;

    // check to make sure there is actually a node group selected
    if (selectedNodes().length() != 1 ||
        selectedNodes()[0]->nodeKind() != NodeKind::NodeGroup)
      return;

    sessionManager->saveNodeGroup((NodeGroup*)selectedNodes()[0],
                                  dialog.selectedFiles().first());
  });

  this->connect(dataflowRenameBox, &QLineEdit::editingFinished, [=] {
    // retrieve pointer to the node we want to rename
    Node* node = dataflowRenameBox->property("node").value<Node*>();

    node->setManualDisplayName(
        std::make_tuple(true, dataflowRenameBox->text()));
    dataflowRenameBox->setVisible(false);
  });

  contextMenu->addSeparator();

  this->contextMenuActionRunFilters = contextMenu->addAction(
      QIcon(":/icons-voxie/play.png"), "&Run all filters");
  connect(this->contextMenuActionRunFilters, &QAction::triggered, this,
          [] { RunAllFilterOperation::create()->runAll(); });

  connect(dataflowWidget, &GraphWidget::contextMenuRequested, this,
          &SidePanel::showContextMenu);

  // loads the custom UI on double-click object activation
  connect(dataflowWidget, &GraphWidget::nodeActivated, this,
          &SidePanel::loadCustomUi);

  contextMenu->addSeparator();

  connect(dataflowWidget, &GraphWidget::currentNodeGroupChanged, this,
          &SidePanel::onCurrentNodeGroupChanged);
  // force refresh after initialization
  onCurrentNodeGroupChanged(nullptr);
}

SidePanel::~SidePanel() {}

void SidePanel::addNode(vx::Node* obj) {
  auto hasDataSection = createQSharedPointer<bool>(false);
  connect(obj, &Node::propertySectionAdded, this,
          [this, hasDataSection, obj](QWidget* section) {
            addSection(section, false, obj);
            *hasDataSection = true;
          });
  for (auto section : obj->propertySections()) {
    addSection(section, false, obj);
    *hasDataSection = true;
  }
  // adds the custom ui widget for the object to the side panel list
  if (obj->getCustomUi()) {
    QWidget* widget = obj->getCustomUi();
    addSection(widget, false, obj, true);
  }
}

void SidePanel::loadCustomUi(vx::Node* node) {
  auto customUi = node->getCustomUi();
  if (!customUi) {
    // Currently this will be called for every double click on a node
    // qWarning() << "SidePanel::loadCustomUi: No custom UI";
    return;
  }

  this->dataflowContainer->hide();
  auto graphNode = dataflowWidget->map[node];
  graphNode->setCustomUiState(true);

  for (auto section : visibleSections) {
    if (!section) continue;
    if (section->property("dockWidget").value<QWidget*>())
      section->property("dockWidget").value<QWidget*>()->setVisible(false);
  }
  visibleSections.clear();

  visibleSections << customUi;
  if (customUi->property("dockWidget").value<QWidget*>())
    customUi->property("dockWidget").value<QWidget*>()->setVisible(true);

  isCustomUiOpen = true;
  Q_EMIT voxieRoot().activeVisualizerProvider()->customUiOpened(node);
}

void SidePanel::closeCustomUi(vx::Node* node) {
  this->dataflowContainer->show();
  isCustomUiOpen = false;

  // TODO: Why is this sometimes called with node == nullptr?
  if (node) {
    auto graphNode = dataflowWidget->map[node];
    if (graphNode) graphNode->setCustomUiState(false);
    Q_EMIT voxieRoot().activeVisualizerProvider()->customUiClosed(node);
  }
  performNodeSelection();
}

void SidePanel::performNodeSelection() {
  // TODO: Why does the custom UI depend on the selected nodes?

  if (selectedNodes().size() == 1) {
    auto obj = selectedNodes()[0];
    auto node = dataflowWidget->map[obj];

    // Skip the side panel changes when custom UI is currently displayed
    if (node && node->getCustomUiState()) {
      return;
    }
  }

  if (isCustomUiOpen) {
    // close custom sidepanel Ui if a different node is selected
    closeCustomUi();
    // Continue with normal side panel code
  }

  for (auto section : visibleSections) {
    if (!section) continue;
    if (section->property("dockWidget").value<QWidget*>())
      section->property("dockWidget").value<QWidget*>()->setVisible(false);
  }
  visibleSections.clear();
  if (selectedNodes().size() == 1) {
    auto obj = selectedNodes()[0];
    for (auto section : obj->propertySections()) {
      visibleSections << section;
      if (section->property("dockWidget").value<QWidget*>())
        section->property("dockWidget").value<QWidget*>()->setVisible(true);
    }
  }
}

void SidePanel::showContextMenu(QPoint globalPos) {
  auto selectedNodes = dataflowWidget->selectedNodes();

  bool haveZeroNodes = selectedNodes.length() == 0;
  bool haveOneNode = selectedNodes.length() == 1;
  bool haveNodes = selectedNodes.length() != 0;

  if (haveOneNode) {
    currentNode = selectedNodes[0];
  } else {
    currentNode = nullptr;
  }

  contextMenuActionDataNodeOpen->setVisible(haveZeroNodes);
  contextMenuActionDataNode->setVisible(haveZeroNodes);
  contextMenuActionFilterNode->setVisible(haveZeroNodes);
  contextMenuActionPropertyNode->setVisible(haveZeroNodes);
  contextMenuActionVisualizerNode->setVisible(haveZeroNodes);
  contextMenuActionObject3DNode->setVisible(haveZeroNodes);
  contextMenuActionRunFilters->setVisible(haveZeroNodes);
  contextMenuActionDelete->setVisible(haveNodes);
  contextMenuActionRename->setVisible(haveNodes);
  contextMenuActionDuplicate->setVisible(haveNodes);
  contextMenuActionMoveToNodeGroup->setVisible(haveNodes);
  contextMenuActionNodeGroup->setVisible(haveZeroNodes);

  contextMenuActionBringToFront->setVisible(false);
  contextMenuActionSaveNodeGroup->setVisible(false);

  if (haveOneNode) {
    auto node = selectedNodes.at(0);

    contextMenuActionSaveNodeGroup->setVisible(node->nodeKind() ==
                                               NodeKind::NodeGroup);
    contextMenuActionDataNode->setVisible(
        node->isCreatableChild(NodeKind::Data));
    contextMenuActionFilterNode->setVisible(
        node->isCreatableChild(NodeKind::Filter));
    contextMenuActionPropertyNode->setVisible(
        node->isCreatableChild(NodeKind::Property));
    contextMenuActionVisualizerNode->setVisible(
        node->isCreatableChild(NodeKind::Visualizer));
    contextMenuActionObject3DNode->setVisible(
        node->isCreatableChild(NodeKind::Object3D));

    contextMenuActionBringToFront->setVisible(
        dynamic_cast<VisualizerNode*>(node));

    // make "bring to front" context menu option visible if we have 1 node
    // selected and it is a visualizer
    contextMenuActionBringToFront->setVisible(
        dynamic_cast<VisualizerNode*>(node));
  }

  auto actions = contextMenu->actions();
  QList<QObject*> objectsToDestroy;  // TODO: exception safety?
  if (haveOneNode) {
    auto node = selectedNodes.at(0);

    actions.append(node->contextMenuActions());

    // TODO: separator?

    for (const auto& tool : vx::Root::instance()
                                ->allComponents()
                                ->listComponentsTyped<vx::Tool>()) {
      auto toolNode = qSharedPointerDynamicCast<ToolTargetNode>(tool);
      if (!toolNode) continue;

      if (!toolNode->matches(node->prototype())) continue;

      auto action = new QAction(tool->displayName(), this);
      objectsToDestroy << action;
      QObject::connect(action, &QAction::triggered, node,
                       [node, toolNode]() { toolNode->startNode(node); });
      actions << action;
    }
  }

  // Open the menu only if there is a visible action
  bool found = false;
  for (const auto& action : actions) {
    if (action->isVisible() && !action->isSeparator()) {
      found = true;
      break;
    }
  }
  if (!found) {
    for (const auto& objD : objectsToDestroy) objD->deleteLater();
    return;
  }

  // contextMenu->popup(globalPos);
  auto newMenu = new QMenu(vx::Root::instance()->mainWindow());
  for (const auto& objD : objectsToDestroy)
    QObject::connect(newMenu, &QObject::destroyed, objD, &QObject::deleteLater);
  QObject::connect(newMenu, &QMenu::aboutToHide, newMenu,
                   &QObject::deleteLater);
  newMenu->addActions(actions);
  // Shift context menu by 2x2 pixels to prevent the user from accidentally
  // selecting the first entry while rightclicking
  QPoint shift(std::ceil(2 / 96.0 * this->logicalDpiX()),
               std::ceil(2 / 96.0 * this->logicalDpiY()));
  newMenu->popup(globalPos + shift);
  if (false) {
    qDebug() << "New menu" << newMenu;
    QObject::connect(newMenu, &QObject::destroyed,
                     [newMenu] { qDebug() << "Destroy menu" << newMenu; });
  }
}

QWidget* SidePanel::addSection(QWidget* section, bool closeable, vx::Node* obj,
                               bool customUi) {
  if (section == nullptr) {
    return nullptr;
  }

  QString title = section->windowTitle();
  if (title.length() == 0) {
    qDebug() << "Window title not set for section" << section;
  }

  QWidget* dockWidget = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  {
    QWidget* headerContainer = new QWidget();
    headerContainer->setStyleSheet(
        "QWidget { background-color: gray; color: white }");
    {
      QHBoxLayout* headerBox = new QHBoxLayout();
      headerBox->setMargin(0);
      headerBox->setSpacing(0);
      {
        auto spaceX = 24 / 96.0 * this->logicalDpiX();
        auto spaceY = 24 / 96.0 * this->logicalDpiY();
        QSpacerItem* spacer =
            new QSpacerItem(spaceX + (closeable ? spaceX : 0), spaceY,
                            QSizePolicy::Minimum, QSizePolicy::Minimum);
        headerBox->addSpacerItem(spacer);

        QLabel* header = new QLabel("<b>" + title + "</b>");
        connect(section, &QWidget::windowTitleChanged, header, [=]() {
          header->setText("<b>" + section->windowTitle() + "</b>");
        });
        header->setAlignment(Qt::AlignCenter);
        headerBox->addWidget(header);

        // button for custom ui widget in sidepanel to return to main view
        if (customUi) {
          auto customUiButton = new ButtonLabel();
          customUiButton->setPixmap(QPixmap(":/icons/arrow-return.png"));
          connect(customUiButton, &ButtonLabel::clicked, this,
                  [=]() { closeCustomUi(obj); });
          connect(obj, &QObject::destroyed, this, [=]() { closeCustomUi(); });

          headerBox->addWidget(customUiButton);
        } else {
          if (closeable) {
            ButtonLabel* closeButton = new ButtonLabel();
            closeButton->setPixmap(QPixmap(":/icons/cross-script.png"));
            connect(closeButton, &ButtonLabel::clicked, dockWidget,
                    &QObject::deleteLater);
            headerBox->addWidget(closeButton);
          }

          ButtonLabel* hideButton = new ButtonLabel();
          hideButton->setPixmap(QPixmap(":/icons/chevron.png"));
          connect(
              hideButton, &ButtonLabel::clicked,
              [hideButton, section]() -> void {
                section->setVisible(!section->isVisible());
                if (section->isVisible()) {
                  hideButton->setPixmap(QPixmap(":/icons/chevron.png"));
                } else {
                  hideButton->setPixmap(QPixmap(":/icons/chevron-expand.png"));
                }
              });
          headerBox->addWidget(hideButton);
        }
      }
      headerContainer->setLayout(headerBox);
    }
    layout->addWidget(headerContainer);
    layout->addWidget(section);
  }
  dockWidget->setLayout(layout);
  dockWidget->setWindowTitle(title);
  connect(section, &QObject::destroyed, dockWidget, &QAction::deleteLater);
  dockWidget->setProperty("section", QVariant::fromValue(section));
  section->setProperty("dockWidget", QVariant::fromValue(dockWidget));
  this->sections->addWidget(dockWidget);

  bool visible = !obj || (this->dataflowWidget->selectedNodes().length() == 1 &&
                          this->dataflowWidget->selectedNodes()[0] == obj);
  dockWidget->setVisible(visible);
  if (visible) visibleSections << section;

  return dockWidget;
}

void SidePanel::addProgressBar(Operation* operation) {
  if (QThread::currentThread() != qApp->thread()) {
    qCritical() << "SidePanel::addProgressBar called from outside main thread";
    return;
  }
  if (QThread::currentThread() != operation->thread()) {
    qCritical() << "Operation does not belong to main thread";
    return;
  }

  auto widget = new QWidget();
  auto layout = new QVBoxLayout();
  layout->setMargin(0);
  widget->setLayout(layout);
  connect(operation, &QObject::destroyed, widget,
          [widget] { widget->deleteLater(); });
  operation->onFinished(
      widget, [widget](const QSharedPointer<Operation::ResultError>& result) {
        Q_UNUSED(result);
        widget->deleteLater();
      });
  bottomLayout->addWidget(widget);

  auto hbox = new QWidget();
  auto hboxLayout = new QHBoxLayout();
  hboxLayout->setMargin(0);
  hbox->setLayout(hboxLayout);
  layout->addWidget(hbox);

  auto label = new QLabel(operation->description());
  hboxLayout->addWidget(label);
  connect(operation, &Operation::descriptionChanged, label, &QLabel::setText);

  auto cancelButton = new ButtonLabel();
  cancelButton->setPixmap(QPixmap(":/icons/cross-button.png"));
  hboxLayout->addWidget(cancelButton);
  connect(cancelButton, &ButtonLabel::clicked, operation, &Operation::cancel);

  auto bar = new QProgressBar();
  layout->addWidget(bar);

  bar->setMinimum(0);
  bar->setMaximum(1000000);
  connect(operation, &Operation::progressChanged, bar,
          [bar](float value) { bar->setValue((int)(value * 1000000 + 0.5)); });
}

void SidePanel::activateVisualizer(VisualizerNode* visualizer) {
  if (visualizer) {
    auto parent =
        qobject_cast<VisualizerContainer*>(visualizer->mainView()->parent());
    if (!parent) {
      qWarning()
          << "Visualizer main view does not have a VisualizerContainer parent"
          << visualizer->mainView();
    } else {
      parent->activate();
    }
  }
}

QString SidePanel::createColorExplanationString(QColor color,
                                                QString explanation) {
  // Don't use px for font size (is incorrect with High-DPI), use pt or em
  // instead.
  return "<span style='color: rgb(" + QString::number(color.red()) + ", " +
         QString::number(color.green()) + ", " + QString::number(color.blue()) +
         "); " + "font-size: 1.2em;'>&#9632;</span> " + explanation;
}

void SidePanel::reorderNodes() { dataflowWidget->reorderNodes(); }

void SidePanel::setOrientation(Qt::Orientation orientation) {
  splitter->setOrientation(orientation);
}

void SidePanel::onCurrentNodeGroupChanged(NodeGroup* newNodeGroup) {
  toolBarActionExitNodeGroup->setVisible(newNodeGroup != nullptr);
}

void SidePanel::userPromptRenameNode(Node* node) {
  // save node for later so we know which node to rename when user is done
  // inputting text
  dataflowRenameBox->setProperty("node", QVariant::fromValue(node));

  dataflowRenameBox->setText(node->displayName());
  dataflowRenameBox->setVisible(true);
  dataflowRenameBox->setFocus();
}
