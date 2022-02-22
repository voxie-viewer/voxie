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

#include <QGraphicsItem>
#include <QList>
#include <QPointer>
#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeNodeProperty.hpp>

class QGraphicsSceneMouseEvent;

namespace vx {
namespace gui {
class Edge;
class GraphWidget;
class GraphNode;

// TODO: Rename to GraphNode.hpp

class IODot {
 public:
  int height = 20;
  int width = 20;

  IODot(GraphNode* graphNode) : graphNode_(graphNode) {}
  virtual ~IODot() {}

  void setPos(QPoint newPos) { pos_ = newPos; }
  void setProperty(NodeNodeProperty property) { property_ = property; }
  QPoint pos() { return pos_; }
  virtual GraphNode* graphNode() { return graphNode_; }
  virtual int slot() = 0;
  virtual QIcon icon() = 0;
  NodeNodeProperty property() { return property_; }
  Node* propertyOrModelNode();

  /**
   * @brief use if property-type is not a list
   *
   * @returns the Object set as the ObjectProperty of the dot
   */
  Node* getPropertyAsObject() {
    if (property_.property != nullptr)
      return vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
          property_.node->getNodePropertyTyped<QDBusObjectPath>(
              property_.property));
    else
      return nullptr;
  }

  /**
   * @brief used if property-type is ObjectReferenceList
   *
   * @returns returns the List of Objects set as the ObjectProperties of the dot
   */
  QList<Node*> getPropertyAsObjectList() {
    if (property_.property)
      return vx::PropertyValueConvertRaw<QList<QDBusObjectPath>,
                                         QList<vx::Node*>>::
          fromRaw(property_.node->getNodePropertyTyped<QList<QDBusObjectPath>>(
              property_.property));
    else
      return QList<Node*>();
  }

 private:
  NodeNodeProperty property_;

  GraphNode* graphNode_;
  QPoint pos_;
};

class InputDot : public IODot {
 public:
  InputDot(GraphNode* node) : IODot(node) {}
  void paint(QPainter* painter);

  /**
   * calculate the "slot-number" of the inputDot by iterating the
   * input-properties of the node
   */
  int slot() override;

  /**
   * get the icon matching the property of the inputDot
   */
  QIcon icon() override;
};

class OutputDot : public IODot {
 public:
  OutputDot(GraphNode* node) : IODot(node) {}
  void paint(QPainter* painter);

  /**
   * calculate the "slot-number" of the OutputDot by iterating the
   * output-properties of the node
   */
  int slot() override;

  QIcon icon() override;
};

/**
 * @brief The GraphNode class is the graphical representation of any node in the
 * dataflow graph
 */
class GraphNode : public QObject, public QGraphicsItem {
 public:
  int width = 140;
  int edgeOffset = 20;
  int height = 40;
  bool newEdgeAction = false;

  GraphNode(GraphWidget* graphWidget);

  /**
   * @brief returns the number of possible inputs of a node by iterating node
   * properties and filtering non-inputs
   *
   * @return the number of inputs
   */
  int getInputCount();

  /**
   * @brief returns the number of possible output of a node by iterating node
   * properties and filtering out inputs
   *
   * @return the number of outputs
   */
  int getOutputCount();

  /*
   * calculate Input- and OutputDots of Node
   * and add them to their matching list of the Node
   */
  void populateIODots();

  /**
   * @brief addEdge adds an edge to the list of edges of this node
   * @param edge
   */
  void addEdge(Edge* edge);

  enum { Type = UserType + 1 };
  int type() const override { return Type; }

  /**
   * @brief calculateForces is a placeholder function for possible later
   * automatic adaption of dataflow graphs
   */
  void calculateForces();

  /**
   * @brief advanceNode returns whether this node changed and needs an
   * graphicall update
   * @return
   */
  bool advanceNode();

  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;
  /**
   * return the icon associated with the node of the graphNode
   */
  QIcon getIcon() { return this->modelNode()->icon(); }

  /**
   * updates the positions of the inputDots in inputDotList of Node
   * to their current position in the GraphicWidget
   */
  void updateInputPos();

  /**
   * updates the positions of the outputDots in outputDotList of Node
   * to their current position in the GraphicWidget
   */
  void updateOutputPos();

  /**
   * @brief getInputPos returns the positions of the incoming ports of the node
   * @return
   */
  QList<QPoint> getInputPos();

  /**
   * @brief getOutputPos returns the positions of the outgoing port of the node
   * @return
   */
  QList<QPoint> getOutputPos();

  void setModelNode(vx::Node* node);
  vx::Node* modelNode() { return this->modelNode_; }
  GraphWidget* graph() { return this->graph_; }
  QList<InputDot*> getInputDotList() { return this->inputDotList; }
  QList<OutputDot*> getOutputDotList() { return this->outputDotList; }

  void setCustomUiState(bool state) { this->customUiState = state; }
  bool getCustomUiState() { return this->customUiState; }

  QString title() { return this->title_; }

 protected:
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant& value) override;

  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

 private:
  /**
   * @brief boolean flag to signal if custom Ui or normal Ui is shown in
   * sidepanel when selecting this node
   */
  bool customUiState = false;
  QPointF newPos;
  GraphWidget* graph_;
  QString title_;
  QPointer<vx::Node> modelNode_;
  QList<InputDot*> inputDotList;
  QList<OutputDot*> outputDotList;
  float iconPadding =
      height / 1.5 +
      8;  // used for correctly aligning node-text to the node-icon

  void drawTextOnRect(QPainter* painter);

  void onChildrenExportedPropertiesChanged();

  /**
   * @brief getDisplayTitle Shortens the title to the nodes width.
   * @param font The QFont in which the title should be displayed.
   * @return the string with a length adapted to the node width
   */
  QString getDisplayTitle(QFont font);

  /**
   * @brief getStaticDisplayTitle Shortens the title to the nodes width.
   * @param font The QFont in which the title should be displayed.
   * @return the string with a length adapted to the node width
   */
  QString getStaticDisplayTitle(QFont font);

  /**
   * @brief shortenStringToNodeWidth Shortens the title to the nodes width.
   * @param string The string to be shortened.
   * @param font The QFont used for the string.
   * @return Returns the string shortened to width of the node. Adds
   * "..." at the end if necessay. Helper Function getDisplayTitle and
   * getStaticDisplayTitle
   */
  QString shortenStringToNodeWidth(QString string, QFont font);
};
}  // namespace gui
}  // namespace vx
