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

#include "GraphWidget.hpp"

#include <math.h>
#include <Main/Root.hpp>
#include <QApplication>
#include <QGraphicsItem>
#include <QMessageBox>

#include <Main/Gui/SidePanel.hpp>

#include "DataFlowScene.hpp"
#include "Edge.hpp"

#include <Voxie/Node/Types.hpp>

#include <QtGui/QKeyEvent>

#include <QtWidgets/QToolTip>

using namespace vx::gui;

// Note: One "pixel" in the graph widget is always assumed to be at 96 dpi, even
// for High-DPI displays. (High-DPI displays use different scaling.)
// This make the drawing code simpler and is also required because the pixel
// positions are saved in the project files.

/**
 * @brief Helper function to check if a QList of QSharedPointers contains a raw
 * pointer.
 */
template <typename T>
bool listContainsPointer(QList<QSharedPointer<T>> list, T* item) {
  for (QSharedPointer<T> child : list) {
    if (child.data() == item) return true;
  }

  return false;
}

GraphWidget::GraphWidget(vx::Root* root, vx::gui::SidePanel* sidePanel,
                         QWidget* parent)
    : QGraphicsView(parent), timerId(0) {
  myScene = new DataFlowScene(this);
  myScene->setItemIndexMethod(QGraphicsScene::NoIndex);
  setScene(myScene);
  setCacheMode(CacheNone);
  setViewportUpdateMode(BoundingRectViewportUpdate);
  setRenderHint(QPainter::Antialiasing);
  setTransformationAnchor(AnchorUnderMouse);
  scaleViewLog(0);
  setMouseTracking(true);

  this->edgeColor = QApplication::palette().text().color();
  if (false) {
    // Use old tool tip implementation
    // Note: This implementation has problems with the scenes bounding box being
    // resized when tool tips are shown
    this->toolTip = new QGraphicsTextItem;
    this->toolTip->setZValue(100);
  }

  this->sidePanel = sidePanel;
  this->root = root;
  autoSort = false;

  connect(root, &Root::nodeAdded, this, &GraphWidget::addNode);

  connect(sidePanel, &SidePanel::nodeSelectionChanged, [=] {
    for (auto item : this->items()) {
      item->update();
    }
  });
  for (auto obj : root->nodes()) addNode(obj.data());

  Q_EMIT currentNodeGroupChanged(currentNodeGroup_);
}

void GraphWidget::paintEvent(QPaintEvent* event) {
  for (Edge* edge : edges()) {
    edge->setVisible(edge->sourceNode()->modelNode()->parentNodeGroup() ==
                     currentNodeGroup());
  }

  QGraphicsView::paintEvent(event);
}

void GraphWidget::addNode(vx::Node* obj) {
  // object without graph visibility shall not be displayed
  // TODO: currently only connect parent with childs, how to handle multiple
  // object connectors?
  if (obj->getGraphVisibility() == false) {
    for (auto parent : obj->parentNodes()) {
      for (auto child : obj->childNodes()) {
        if (!map.contains(child) || !map.contains(parent)) continue;
        auto edge = new vx::gui::Edge(map[parent]->getOutputDotList()[0],
                                      map[child]->getInputDotList()[0]);
        edge->setZValue(0);
        myScene->addItem(edge);
      }
    }
    return;
  }
  connect(obj, &Node::childChanged, this, &GraphWidget::addNode,
          Qt::UniqueConnection);
  connect(obj, &Node::parentChanged, this, &GraphWidget::addNode,
          Qt::UniqueConnection);

  GraphNode* node0 = map[obj];
  // TODO: This check looks weird
  if (!map.contains(obj) || node0 == nullptr) {
    // TODO: This object does not seem to be deleted anywhere (possibly also a
    // problem for other values allocated in this class)
    auto node = new vx::gui::GraphNode(this);
    node->setZValue(10);
    myScene->addItem(node);
    node->setModelNode(obj);

    QPointF spawnPlace = QPointF();
    if (obj->parentNodes().size() != 0) {
      for (auto parent : obj->parentNodes()) {
        spawnPlace.setX(map[parent]->pos().x());
        spawnPlace.setY(map[parent]->pos().y());
      }

      spawnPlace.setX(spawnPlace.x() + 200);
    }

    bool placeIsFree = true;
    int minDistY = 100;
    do {
      placeIsFree = true;
      for (QGraphicsItem* item : scene()->items()) {
        if (GraphNode* tnode = qgraphicsitem_cast<GraphNode*>(item)) {
          auto distanceVec = (tnode->pos() - spawnPlace);
          if (std::abs(distanceVec.x()) < 150 &&
              std::abs(distanceVec.y()) < minDistY) {
            spawnPlace.setY(spawnPlace.y() + minDistY);
            placeIsFree = false;
            break;
          }
        }
      }
    } while (!placeIsFree);

    node->setPos(spawnPlace);

    if (obj->parentNodes().size() == 0 && this->isVisible() &&
        !this->lastClickPos.isNull()) {
      node->setPos(this->lastClickPos);
    }

    map[obj] = node;

    connect(obj, &QObject::destroyed, this, [this, obj] {
      for (Edge* edge : edges()) {
        if (edge->sourceNode()->modelNode() == obj ||
            edge->destNode()->modelNode() == obj) {
          myScene->removeItem(edge);
          delete edge;
        }
      }

      GraphNode* node2 = map[obj];
      if (node2->scene() != nullptr) {
        myScene->removeItem(node2);
      }
      map.remove(obj);
    });
  }

  for (auto edge : edges()) {
    if (edge->sourceNode()->modelNode() == obj ||
        edge->destNode()->modelNode() == obj) {
      myScene->removeItem(edge);
      delete edge;
    }
  }

  for (auto parent : obj->parentNodes()) {
    if (!map[parent]) {
      qWarning() << "No GraphNode found for parent object" << parent;
      continue;
    }

    if (parent->parentNodeGroup() == obj->parentNodeGroup()) {
      // if parent and obj are in the same node group we can connect the
      // objects directly
      int outputDotSlot = 0;
      for (OutputDot* outputDot : map[parent]->getOutputDotList()) {
        if (outputDot->property().property != nullptr) {
          if (outputDot->property().property->type() !=
              vx::types::NodeReferenceListType()) {
            if (outputDot->getPropertyAsObject() == obj) {
              outputDotSlot = outputDot->slot();
              break;
            }
          } else {
            if (outputDot->getPropertyAsObjectList().contains(obj)) {
              outputDotSlot = outputDot->slot();
              break;
            }
          }
        }
      }

      for (InputDot* inputDot : map[obj]->getInputDotList()) {
        if (inputDot->property().property != nullptr) {
          if (inputDot->property().property->type() !=
              vx::types::NodeReferenceListType()) {
            if (inputDot->getPropertyAsObject() == parent) {
              Edge* edge = new vx::gui::Edge(
                  map[parent]->getOutputDotList()[outputDotSlot],
                  map[obj]->getInputDotList()[inputDot->slot()]);
              edge->setZValue(0);
              myScene->addItem(edge);
            }

          } else {
            if (inputDot->getPropertyAsObjectList().contains(parent)) {
              Edge* edge = new vx::gui::Edge(
                  map[parent]->getOutputDotList()[outputDotSlot],
                  map[obj]->getInputDotList()[inputDot->slot()]);
              edge->setZValue(0);
              myScene->addItem(edge);
            }
          }

        } else {
          Edge* edge =
              new vx::gui::Edge(map[parent]->getOutputDotList()[outputDotSlot],
                                map[obj]->getInputDotList()[0]);
          edge->setZValue(0);
          myScene->addItem(edge);
        }
      }

    } else {
      // if parent and obj are not in the same node group we will check if
      // parent is in the same subtree as obj. If it is not we go one "layer"
      // higher (so to obj's parentNodeGroup) and check again until we find the
      // subtree that contains both nodes.
      bool foundParentGroup = false;
      NodeGroup* objectsParent = obj->parentNodeGroup().data();
      while (!foundParentGroup) {
        QList<QSharedPointer<Node>> groupChildren;

        if (objectsParent == nullptr) {
          groupChildren = vx::voxieRoot().nodes();
        } else {
          groupChildren = objectsParent->groupChildren(true);
        }

        for (auto groupChild : groupChildren) {
          if (groupChild.data() == parent) {
            // we have now found the subtree that contains both nodes. Now we
            // have to find out which two nodes of the topmost layer of the
            // subtree represent obj and parent.
            Node* objRepr;
            Node* parentRepr;

            QList<QSharedPointer<Node>> layerChildren;
            if (objectsParent == nullptr) {
              for (auto node : vx::voxieRoot().nodes()) {
                if (node->parentNodeGroup() == nullptr) {
                  layerChildren.append(node);
                }
              }

            } else {
              layerChildren = objectsParent->groupChildren();
            }

            for (QSharedPointer<Node> layerChild : layerChildren) {
              NodeGroup* layerGroup =
                  dynamic_cast<NodeGroup*>(layerChild.data());
              if (layerGroup) {
                if (listContainsPointer(layerGroup->groupChildren(), obj)) {
                  objRepr = layerGroup;
                }
                if (listContainsPointer(layerGroup->groupChildren(), parent)) {
                  parentRepr = layerGroup;
                }

              } else {
                if (layerChild == obj) {
                  objRepr = layerChild.data();
                }

                if (layerChild == parent) {
                  parentRepr = layerChild.data();
                }
              }
            }

            InputDot* inputDot = nullptr;
            for (InputDot* x : map[objRepr]->getInputDotList()) {
              if (x->getPropertyAsObject() == obj) {
                inputDot = x;
              }
            }

            if (inputDot) {
              Edge* edge = new vx::gui::Edge(
                  map[parentRepr], map[parentRepr]->getOutputDotList()[0],
                  map[objRepr], inputDot);
              edge->setZValue(0);
              myScene->addItem(edge);
            }
            foundParentGroup = true;
            break;
          }
        }

        if (!foundParentGroup) {
          objectsParent = objectsParent->parentNodeGroup().data();
        }
      }
    }
  }
  for (auto child : obj->childNodes()) {
    if (!map[child]) {
      qWarning() << "No GraphNode found for child object" << child;
      continue;
    }

    if (child->parentNodeGroup() == obj->parentNodeGroup()) {
      // if child and obj are in the same node group we can connect the
      // objects directly
      int outputDotSlot = 0;
      for (OutputDot* outputDot : map[obj]->getOutputDotList()) {
        if (outputDot->property().property != nullptr) {
          if (outputDot->property().property->type() !=
              vx::types::NodeReferenceListType()) {
            if (outputDot->getPropertyAsObject() == child) {
              outputDotSlot = outputDot->slot();
              break;
            }
          } else {
            if (outputDot->getPropertyAsObjectList().contains(child)) {
              outputDotSlot = outputDot->slot();
              break;
            }
          }
        }
      }

      for (InputDot* inputDot : map[child]->getInputDotList()) {
        if (inputDot->property().property != nullptr) {
          if (inputDot->property().property->type() !=
              vx::types::NodeReferenceListType()) {
            if (inputDot->getPropertyAsObject() == obj) {
              Edge* edge = new vx::gui::Edge(
                  map[obj]->getOutputDotList()[outputDotSlot],
                  map[child]->getInputDotList()[inputDot->slot()]);
              edge->setZValue(0);
              myScene->addItem(edge);
            }

          } else {
            if (inputDot->getPropertyAsObjectList().contains(obj)) {
              Edge* edge = new vx::gui::Edge(
                  map[obj]->getOutputDotList()[outputDotSlot],
                  map[child]->getInputDotList()[inputDot->slot()]);
              edge->setZValue(0);
              myScene->addItem(edge);
            }
          }

        } else {
          Edge* edge =
              new vx::gui::Edge(map[obj]->getOutputDotList()[outputDotSlot],
                                map[child]->getInputDotList()[0]);
          edge->setZValue(0);
          myScene->addItem(edge);
        }
      }

    } else {
      // if child and obj are not in the same node group we will check if
      // child is in the same subtree as obj. If it is not we go one "layer"
      // higher (so to obj's parentNodeGroup) and check again until we find the
      // subtree that contains both nodes.
      bool foundParentGroup = false;
      NodeGroup* objectsParent = obj->parentNodeGroup().data();
      while (!foundParentGroup) {
        QList<QSharedPointer<Node>> groupChildren;

        if (objectsParent == nullptr) {
          groupChildren = vx::voxieRoot().nodes();
        } else {
          groupChildren = objectsParent->groupChildren(true);
        }

        for (auto groupChild : groupChildren) {
          if (groupChild.data() == child) {
            // we have now found the subtree that contains both nodes. Now we
            // have to find out which two nodes of the topmost layer of the
            // subtree represent obj and parent.
            Node* objRepr;
            Node* childRepr;

            QList<QSharedPointer<Node>> layerChildren;
            if (objectsParent == nullptr) {
              for (auto node : vx::voxieRoot().nodes()) {
                if (node->parentNodeGroup() == nullptr) {
                  layerChildren.append(node);
                }
              }

            } else {
              layerChildren = objectsParent->groupChildren();
            }

            for (QSharedPointer<Node> layerChild : layerChildren) {
              NodeGroup* layerGroup =
                  dynamic_cast<NodeGroup*>(layerChild.data());
              if (layerGroup) {
                if (listContainsPointer(layerGroup->groupChildren(), obj)) {
                  objRepr = layerGroup;
                }
                if (listContainsPointer(layerGroup->groupChildren(), child)) {
                  childRepr = layerGroup;
                }

              } else {
                if (layerChild == obj) {
                  objRepr = layerChild.data();
                }

                if (layerChild == child) {
                  childRepr = layerChild.data();
                }
              }
            }

            InputDot* inputDot = nullptr;
            for (InputDot* x : map[childRepr]->getInputDotList()) {
              if (x->getPropertyAsObject() == obj) {
                inputDot = x;
                break;
              }
            }

            if (inputDot) {
              Edge* edge = new vx::gui::Edge(
                  map[objRepr], map[objRepr]->getOutputDotList()[0],
                  map[childRepr], inputDot);
              edge->setZValue(0);
              myScene->addItem(edge);
            }
            foundParentGroup = true;
            break;
          }
        }

        if (!foundParentGroup) {
          objectsParent = objectsParent->parentNodeGroup().data();
        }
      }
    }
  }

  // new node should only be visible if it is in the currently opened node
  // group
  connect(obj, &vx::Node::parentNodeGroupChanged, this,
          [obj, this](NodeGroup* newNodeGroup) {
            map[obj]->setVisible(newNodeGroup == currentNodeGroup_);
          });

  if (autoSort) {
    reorderNodes();
  }
}

/*following code is new added by meng, and also conresponding in hpp file*/
GraphWidget::~GraphWidget() {
  if (this->root != nullptr) {
    this->root->deleteLater();
  }
}

void GraphWidget::graphNodeActivated(vx::gui::GraphNode* node) {
  if (node->modelNode()->nodeKind() == NodeKind::NodeGroup) {
    setCurrentNodeGroup((NodeGroup*)node->modelNode());
  }

  Q_EMIT nodeActivated(node->modelNode());
}

void GraphWidget::connectNodes(OutputDot* srcDot, InputDot* dstDot) {
  Node* srcPropNode = srcDot->propertyOrModelNode();
  Node* dstPropNode = dstDot->propertyOrModelNode();

  if (dstDot->property().property &&
      dstDot->property().property->allowsAsValue(srcPropNode)) {
    if (dstDot->property().property->type() !=
        vx::types::NodeReferenceListType()) {
      if (dstDot->getPropertyAsObject() == srcPropNode) {
        srcPropNode->removeChildNode(dstPropNode, dstDot->slot());
        return;
      }
    } else {
      if (dstDot->getPropertyAsObjectList().contains(srcPropNode)) {
        srcPropNode->removeChildNode(dstPropNode, dstDot->slot());
        return;
      }
    }
    srcPropNode->addChildNode(dstPropNode, dstDot->slot());

  } else if (srcDot->property().property &&
             srcDot->property().property->allowsAsValue(
                 dstDot->graphNode()->modelNode()->prototype())) {
    srcPropNode->addChildNode(dstDot->graphNode()->modelNode(), -1);

  } else {
    qWarning() << "Selected Nodes can't be connected";
  }
}

void GraphWidget::connectNodes(OutputDot* srcDot, GraphNode* dstNode) {
  Node* srcPropNode = srcDot->propertyOrModelNode();

  if (srcPropNode->isCreatableChild(dstNode->modelNode()->nodeKind())) {
    for (InputDot* inputDot : dstNode->getInputDotList()) {
      if (inputDot->property().property->type() !=
          vx::types::NodeReferenceListType()) {
        if (inputDot->getPropertyAsObject() == srcPropNode) {
          srcPropNode->removeChildNode(dstNode->modelNode());
          return;
        }

      } else {
        if (inputDot->getPropertyAsObjectList().contains(srcPropNode)) {
          srcPropNode->removeChildNode(dstNode->modelNode());
          return;
        }
      }
    }
    srcPropNode->addChildNode(dstNode->modelNode(), -1);

  } else if (srcDot->property().property->allowsAsValue(
                 dstNode->modelNode()->prototype())) {
    srcPropNode->addChildNode(dstNode->modelNode(), -1);

  } else {
    qWarning() << "Selected Nodes can't be connected";
  }
}

void GraphWidget::itemMoved() {
  if (!timerId) timerId = startTimer(1000 / 25);
}

void GraphWidget::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Plus:
      zoomIn();
      break;
    case Qt::Key_Minus:
      zoomOut();
      break;
    case Qt::Key_Delete: {
      for (auto obj : selectedNodes()) {
        if (obj) obj->destroy();
      }
      // TODO: Should the key press be forwarded to QGraphicsView if there is no
      // node?
    } break;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
      for (auto obj : selectedNodes()) {
        if (obj) Q_EMIT nodeActivated(obj);
      }
    } break;
    case Qt::Key_F2: {
      if (selectedNodes().size() == 1) {
        sidePanel->userPromptRenameNode(selectedNodes()[0]);
      }
    } break;
    default:
      QGraphicsView::keyPressEvent(event);
  }
}

void GraphWidget::mouseMoveEvent(QMouseEvent* pEvent) {
  QPointF MousePos = this->mapToScene(pEvent->pos());

  if (hoverInputDot && hoverInputDot->property().property) {
    if (this->toolTip) {
      this->toolTip->setPos(QPointF(MousePos.x() + 10, MousePos.y() - 20));
      this->toolTip->setHtml(
          QString("<div style='background:#ffffca; font-size: 16px;'>" +
                  hoverInputDot->property().property->displayName() +
                  QString("</div>")));
      if (!myScene->items().contains(toolTip)) myScene->addItem(toolTip);
    } else {
      // Force tooltip movement
      QToolTip::showText(pEvent->globalPos(), "--", this, QRect(), 1);
      QToolTip::showText(pEvent->globalPos(),
                         hoverInputDot->property().property->displayName(),
                         this, QRect(), 1000000000);
    }
  } else if (hoverOutputDot && hoverOutputDot->property().property) {
    if (this->toolTip) {
      this->toolTip->setPos(QPointF(MousePos.x() + 10, MousePos.y() - 20));
      this->toolTip->setHtml(
          QString("<div style='background:#ffffca; font-size: 16px;'>" +
                  hoverOutputDot->property().property->displayName() +
                  QString("</div>")));
      if (!myScene->items().contains(toolTip)) myScene->addItem(toolTip);
    } else {
      // Force tooltip movement
      QToolTip::showText(pEvent->globalPos(), "--", this, QRect(), 1);
      QToolTip::showText(pEvent->globalPos(),
                         hoverOutputDot->property().property->displayName(),
                         this, QRect(), 1000000000);
    }

  } else {
    if (this->toolTip) {
      if (myScene->items().contains(this->toolTip)) {
        myScene->removeItem(this->toolTip);
      }
    } else {
      QToolTip::showText(pEvent->globalPos(), "", this);
    }
  }

  Q_EMIT mousePosChanged(MousePos.toPoint());
  QGraphicsView::mouseMoveEvent(pEvent);
}

void GraphWidget::requestContextMenu(QPoint pos) {
  Q_EMIT contextMenuRequested(pos);
}

void GraphWidget::timerEvent(QTimerEvent* event) {
  Q_UNUSED(event);

  QList<GraphNode*> nodes;
  for (QGraphicsItem* item : scene()->items()) {
    if (GraphNode* node = qgraphicsitem_cast<GraphNode*>(item)) nodes << node;
  }

  for (GraphNode* node : nodes) {
    node->calculateForces();
  }

  bool itemsMoved = false;
  for (GraphNode* node : nodes) {
    if (node->advanceNode()) {
      itemsMoved = true;
    }
  }

  if (!itemsMoved) {
    killTimer(timerId);
    timerId = 0;
  }
}

void GraphWidget::wheelEvent(QWheelEvent* event) {
  scaleViewLog(event->delta() / 240.0);
}

void GraphWidget::drawBackground(QPainter* painter, const QRectF& rect) {
  painter->fillRect(
      rect, QBrush(currentNodeGroup_ == nullptr ? Qt::white : Qt::yellow,
                   Qt::SolidPattern));
}

void GraphWidget::scaleViewLog(double step) {
  // qDebug() << "scaleViewLog" << currentScaleStep << step;
  currentScaleStep = std::min(std::max(currentScaleStep + step, -3.5), 2.5);
  // qDebug() << "scaleViewLog new" << currentScaleStep;

  qreal currentFactor = transform().mapRect(QRectF(0, 0, 1, 1)).width();

  double scaleFactor =
      0.8 / 96.0 * this->logicalDpiY() * pow(2.0, currentScaleStep);

  scale(scaleFactor / currentFactor, scaleFactor / currentFactor);
}

void GraphWidget::zoomIn() { scaleViewLog(0.25); }

void GraphWidget::zoomOut() { scaleViewLog(-0.25); }

static bool debug = false;

void GraphWidget::reorderNodes() {
  QList<GraphNode*> nodes;
  QList<Edge*> allEdges;
  QList<Edge*> edges;
  QList<QList<GraphNode*>> nodeHierarchy;
  int currentLayer = 0;
  int currentLayerY = 0;
  int currentLayerX = 0;
  int spacingY = 100;
  int spacingX = 170;
  int widestLayer = 0;

  // Get lists of all nodes and edges currently in the graph
  for (QGraphicsItem* item : scene()->items()) {
    if (GraphNode* node = qgraphicsitem_cast<GraphNode*>(item)) {
      nodes.append(node);
    }
    if (Edge* edge = qgraphicsitem_cast<Edge*>(item)) {
      edges.append(edge);
    }
  }

  allEdges = edges;

  /* Assign layers to all nodes. In each iteration, nodes without incoming edges
   are assigned to the current layer, and all their outgoing edges are removed
   from the graph. This is step 2 in the algorithm. */

  while (!nodes.isEmpty()) {
    if (currentLayer > 50) {
      qDebug() << "Leftover nodes:";
      for (const auto& node : nodes) qDebug() << " " << node->modelNode();

      QMessageBox(QMessageBox::Warning, "Error",
                  "Algorithm could not finish. Make sure your graph contains "
                  "no cycles.",
                  QMessageBox::Ok, this)
          .exec();
      return;
    }

    nodeHierarchy.append(nodes);
    // Find all nodes in current layer with no inbound edges (parents)
    for (GraphNode* node : nodes) {
      for (Edge* edge : edges) {
        // GraphNode has an inbound edge, so not a parent. Removing.
        if (node == edge->destNode()) {
          if (debug)
            qDebug() << "On layer" << currentLayer << "removing node"
                     << node->modelNode() << "because of edge from"
                     << edge->sourceNode()->modelNode() << "to"
                     << edge->destNode()->modelNode();
          nodeHierarchy[currentLayer].removeAll(node);
          break;
        }
      }
    }
    if (debug) {
      qDebug() << "Nodes on layer" << currentLayer;
      for (const auto& node : nodeHierarchy[currentLayer])
        qDebug() << " " << node->modelNode();
    }

    // Remove all outgoing edges of current layer
    // Note: Iteration has to be done on a copy, removeAll() must not be called
    // on the QList which is passed to for()
    QList<Edge*> edgesCopy = edges;
    for (Edge* edge : edgesCopy) {
      if (nodeHierarchy[currentLayer].contains(edge->sourceNode())) {
        edges.removeAll(edge);
      }
    }

    // We no longer need to consider nodes that already have a layer assigned
    for (GraphNode* node : nodeHierarchy[currentLayer]) {
      nodes.removeAll(node);
    }
    currentLayer++;
  }

  // Layering is done now. Now we position items horizontally layer by layer
  for (int i = 0; i < nodeHierarchy.length(); i++) {
    if (nodeHierarchy[i].length() > 0) {
      // Make a list of nodes paired with the median of their parents
      QList<QPair<int, GraphNode*>> sortedNodes;
      for (GraphNode* node : nodeHierarchy[i]) {
        int median = getMedian(node, nodeHierarchy, allEdges, i);
        sortedNodes.append(qMakePair(median, node));
      }
      std::sort(sortedNodes.begin(), sortedNodes.end());

      // Calculate the position of the first node so that they are centered
      currentLayerX = -sortedNodes.length() * 0.5 * spacingX;

      if (sortedNodes.length() > widestLayer) {
        widestLayer = sortedNodes.length();
      }

      for (const auto& pair : sortedNodes) {
        pair.second->setPos(currentLayerX, currentLayerY);
        currentLayerX += spacingX;
      }
    }
    // Proceeding to next layer, adjust height
    currentLayerY += spacingY;
  }

  // Adapt scrollable area to new graph size
  myScene->setSceneRect(-widestLayer * spacingX * 0.5 - 80, -50,
                        widestLayer * spacingX, spacingY * currentLayer + 50);
}

int GraphWidget::getMedian(GraphNode* node, QList<QList<GraphNode*>> nodes,
                           QList<Edge*> edges, int layer) {
  QVector<int> positions;
  for (Edge* edge : edges) {
    // Find all parent nodes of the node we are looking at
    if (edge->destNode() == node &&
        nodes[layer - 1].contains(edge->sourceNode())) {
      positions.append(edge->sourceNode()->pos().x());
    }
  }

  int size = positions.size();

  if (size == 0) {
    return node->pos().x();
  } else {
    std::sort(positions.begin(), positions.end());
    if (size % 2 == 0) {
      return (positions[size / 2 - 1] + positions[size / 2]) / 2;
    } else {
      return positions[size / 2];
    }
  }
}

void GraphWidget::setAutoReorder(bool enabled) {
  autoSort = enabled;
  if (enabled) reorderNodes();
}

void GraphWidget::selectNode(vx::Node* obj) {
  selectedNodes_.append(obj);
  Q_EMIT selectionChanged(selectedNodes_);
  repaint();
}

void GraphWidget::deselectNode(vx::Node* obj) {
  selectedNodes_.removeOne(obj);
  Q_EMIT selectionChanged(selectedNodes_);
  repaint();
}

void GraphWidget::clearSelectedNodes() {
  selectedNodes_.clear();
  Q_EMIT selectionChanged(selectedNodes_);
  repaint();
}

void GraphWidget::setSelectedNodes(QList<Node*> list) {
  this->selectedNodes_ = list;
  Q_EMIT selectionChanged(selectedNodes_);
  repaint();
}

NodeGroup* GraphWidget::currentNodeGroup() { return currentNodeGroup_; }

void GraphWidget::setCurrentNodeGroup(NodeGroup* nodeGroup) {
  currentNodeGroup_ = nodeGroup;

  for (GraphNode* node : map) {
    node->setVisible(node->modelNode()->parentNodeGroup().data() == nodeGroup);
  }

  Q_EMIT currentNodeGroupChanged(nodeGroup);

  repaint();
}

void GraphWidget::deleteSelectedNodes() {
  for (Node* node : selectedNodes()) {
    node->destroy();
  }

  for (Edge* edge : edges()) {
    if (selectedNodes().contains(edge->sourceNode()->modelNode()) ||
        selectedNodes().contains(edge->destNode()->modelNode())) {
      scene()->removeItem(edge);
      delete edge;
    }
  }

  if (selectedNodes().contains(currentNodeGroup_)) {
    setCurrentNodeGroup(currentNodeGroup_->parentNodeGroup());
  }

  clearSelectedNodes();
}

QList<Edge*> GraphWidget::edges() {
  QList<Edge*> result;
  for (QGraphicsItem* item : scene()->items()) {
    Edge* edge = dynamic_cast<Edge*>(item);
    if (edge) {
      result.append(edge);
    }
  }

  return result;
}
