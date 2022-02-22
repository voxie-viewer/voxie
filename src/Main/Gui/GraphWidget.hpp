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

#include <QGraphicsView>
#include <QtCore/QPointer>
#include <Voxie/Node/NodeGroup.hpp>
#include "Node.hpp"

namespace vx {
class Root;
class Node;
namespace gui {
class SidePanel;
}  // namespace gui
}  // namespace vx

namespace vx {
namespace gui {
/**
 * @brief The GraphWidget class is the parent widget for the whole dataflow
 * graph
 */
class GraphWidget : public QGraphicsView {
  Q_OBJECT

  vx::Root* root;

 public:
  GraphWidget(vx::Root* root, vx::gui::SidePanel* sidePanel,
              QWidget* parent = 0);

  ~GraphWidget();

  /**
   * @brief itemMoved is called when an item was moved
   */
  void itemMoved();

  /**
   * @brief map stores a mapping between the data model Nodes and the
   * graphical representation = nodes
   */
  QMap<vx::Node*, GraphNode*> map;
  QGraphicsScene* myScene;
  QPointF lastClickPos = QPointF();

  /**
   * @brief addNode should be called when a node was added or a connection
   * of a node was changed
   * @param obj Node which was added or updated
   */
  void addNode(vx::Node* obj);

  /**
   * @brief Deletes all nodes that are currently selected.
   */
  void deleteSelectedNodes();

  /**
   * @brief graphNodeSelected is called when a node was selected
   * @param nodelist
   */
  void graphNodeSelected(vx::gui::GraphNode* node);

  /**
   * @brief graphNodeActivated is called when a node was activated (i.e. double
   * clicked)
   * @param node
   */
  void graphNodeActivated(vx::gui::GraphNode* node);

  /**
   * @brief connectNodes connect two nodes from an outputDot to an inputDot
   * @param srcDot the outputDot of the source/parent Node
   * @param dstDot the inputDot of the destination/child Node
   */
  void connectNodes(OutputDot* srcDot, InputDot* dstDot);

  /**
   * @brief connectNodes two nodes by calling the SelectObjectConnection-Frame
   * @param srcDot the outputDot of the source/parent Node
   * @param dstNode the targetNode
   */
  void connectNodes(OutputDot* srcDot, GraphNode* dstNode);

  /**
   * @brief selectedNodes Returns a list containing the selected nodes in the
   * node graph
   */
  QList<vx::Node*> selectedNodes() const { return selectedNodes_; }

  void selectNode(vx::Node* obj);

  void deselectNode(vx::Node* obj);

  void clearSelectedNodes();

  void setSelectedNodes(QList<vx::Node*> list);

  QList<Edge*> edges();

  /**
   * @brief If the graph widget is currently "inside" a node group this function
   * will return a pointer to that node group. Returns nullptr otherwise.
   */
  NodeGroup* currentNodeGroup();

  /**
   * @brief If set to a node group the graph widget will "open" that node group.
   * Set to nullptr to exit node group.
   */
  void setCurrentNodeGroup(NodeGroup* nodeGroup);

  /**
   * @brief mouseMoveEvent called when the mouse position changed
   * @param pEvent
   */
  void mouseMoveEvent(QMouseEvent* pEvent) override;

  /**
   * @brief requestContextMenu signals the sidebar to display the context menu
   * at the given position
   * @param pos for the contextmenu
   */
  void requestContextMenu(QPoint pos);

  /**
   * @brief scaleFactor return current zoom level of the graph widget
   * @return
   */
  qreal scaleFactor();

  /**
   * @brief Reorders all nodes in the graph based on the hierarchical algorithm
   * by Sugiyama and Kozo
   */
  void reorderNodes();

  void setAutoReorder(bool enabled);

  OutputDot* arrowStart = nullptr;
  GraphNode* hoverNode = nullptr;
  InputDot* hoverInputDot = nullptr;
  OutputDot* hoverOutputDot = nullptr;
  QGraphicsTextItem* toolTip = new QGraphicsTextItem;

  const QColor dataNodeColor = QColor(76, 175, 80);
  const QColor filterNodeColor = QColor(33, 150, 243);
  const QColor visualizerNodeColor = QColor(255, 235, 59);
  const QColor propertyNodeColor = QColor(244, 67, 54);
  const QColor object3DNodeColor = QColor(244, 67, 244);
  QColor edgeColor;

  const QColor inputDotValidColor = QColor(27, 103, 224);
  const QColor inputDotInvalidColor = QColor(240, 41, 61);
  const QColor inputDotUnverifiedColor = QColor(204, 204, 0);
  const QColor inputDotAlreadyConnectedColor = QColor(0, 204, 0);

 public Q_SLOTS:
  void zoomIn();
  void zoomOut();

 protected:
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void drawBackground(QPainter* painter, const QRectF& rect) override;

  void scaleView(qreal scaleFactor);

 private:
  int timerId;
  GraphNode* centerNode;
  SidePanel* sidePanel;
  bool autoSort;
  QList<Node*> selectedNodes_;
  NodeGroup* currentNodeGroup_ = nullptr;

  /**
   * @brief Helper function for the graph layouting algorithm
   * @returns Median position of a layer of nodes
   */
  int getMedian(GraphNode* node, QList<QList<GraphNode*>> nodes,
                QList<Edge*> edges, int layer);

 Q_SIGNALS:
  void selectionChanged(QList<vx::Node*> selectedNodes);
  void nodeActivated(vx::Node* obj);
  void mousePosChanged(QPoint point);
  void contextMenuRequested(QPoint pos);
  void currentNodeGroupChanged(NodeGroup* nodeGroup);
};
}  // namespace gui
}  // namespace vx
