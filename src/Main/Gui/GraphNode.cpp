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

#include "GraphNode.hpp"

#include <Main/Gui/DataFlowScene.hpp>
#include <Main/Gui/Edge.hpp>
#include <Main/Gui/GraphWidget.hpp>

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeGroup.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <QtCore/QDebug>

#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>

#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QStyleOption>

using namespace vx::gui;
using namespace vx;

GraphNode::GraphNode(GraphWidget* graphWidget) : graph_(graphWidget) {
  setFlag(ItemIsMovable);
  setFlag(ItemSendsGeometryChanges);
  setCacheMode(DeviceCoordinateCache);
  setZValue(-1);
  setAcceptHoverEvents(true);
  setAcceptDrops(true);
}

Node* IODot::propertyOrModelNode() {
  if (property_.node) {
    return property_.node;
  }

  return graphNode_->modelNode();
}

int InputDot::slot() {
  int propertySlot = 0;
  for (const auto& property :
       propertyOrModelNode()->prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;
    if (this->property().property == property)
      return propertySlot;
    else
      propertySlot++;
  }
  return 0;
}

QIcon InputDot::icon() {
  if (!property().property) return this->graphNode()->getIcon();

  QList<QWeakPointer<vx::NodePrototype>> allowedTypes =
      property().property->allowedTypes();
  if (allowedTypes.size() !=
      1) {  // use universal icon as wildcard for multiple allowed types
    return QIcon(":/icons/universal.png");

  } else {
    auto allowedType = allowedTypes[0].lock();
    if (!allowedType) {
      qWarning() << "Values in allowedTypes[0] has already been destroyed";
      return this->graphNode()->getIcon();
    }
    QString iconName = allowedType->icon();
    if (iconName != "")
      return QIcon(iconName);
    else
      return this->graphNode()->getIcon();
  }
}

void InputDot::paint(QPainter* painter) {
  painter->setPen(this->graphNode()->graph()->edgeColor);
  painter->setBrush(Qt::transparent);
  painter->drawRect(QRect(this->pos(), QPoint(this->pos().x() + this->width,
                                              this->pos().y() + this->height)));
  painter->setOpacity(0.5);
  this->icon().paint(painter, this->pos().x(), this->pos().y(), this->width,
                     this->height, Qt::AlignCenter, QIcon::Normal, QIcon::On);
  painter->setOpacity(1);
}

int OutputDot::slot() {
  int propertySlot = 0;
  for (const auto& property :
       this->graphNode()->modelNode()->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;
    if (this->property().property == property)
      return propertySlot;
    else
      propertySlot++;
  }
  return 0;
}

QIcon OutputDot::icon() {
  if (!property().property) return this->graphNode()->getIcon();

  QList<QWeakPointer<vx::NodePrototype>> allowedTypes =
      property().property->allowedTypes();
  if (allowedTypes.size() !=
      1) {  // use universal icon as wildcard for multiple allowed types
    return QIcon(":/icons/universal.png");

  } else {
    auto allowedType = allowedTypes[0].lock();
    if (!allowedType) {
      qWarning() << "Values in allowedTypes[0] has already been destroyed";
      return this->graphNode()->getIcon();
    }
    QString iconName = allowedType->icon();
    if (iconName != "")
      return QIcon(iconName);
    else
      return this->graphNode()->getIcon();
  }
}

void OutputDot::paint(QPainter* painter) {
  painter->setPen(this->graphNode()->graph()->edgeColor);
  painter->setBrush(Qt::transparent);
  painter->drawRect(QRect(this->pos(), QPoint(this->pos().x() + this->width,
                                              this->pos().y() + this->height)));
  painter->setOpacity(0.5);
  this->icon().paint(painter, this->pos().x(), this->pos().y(), this->width,
                     this->height, Qt::AlignCenter, QIcon::Normal, QIcon::On);
  painter->setOpacity(1);
}

void GraphNode::addEdge(Edge* edge) { edge->adjust(); }

int GraphNode::getInputCount() {
  Node* node = this->modelNode();
  int numOfPossibleInputs = 0;
  // Search for input property in child which allows parent as value
  for (const auto& property : node->prototype()->nodeProperties()) {
    if (!property->isReference() || property->isOutputReference()) continue;
    numOfPossibleInputs++;
  }
  return numOfPossibleInputs;
}

int GraphNode::getOutputCount() {
  Node* node = this->modelNode();
  int numOfPossibleInputs = 0;
  // Search for input property in child which allows parent as value
  for (const auto& property : node->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;
    numOfPossibleInputs++;
  }
  return numOfPossibleInputs;
}

void GraphNode::populateIODots() {
  inputDotList.clear();
  outputDotList.clear();

  auto nodeGroup = dynamic_cast<vx::NodeGroup*>(this->modelNode());

  if (nodeGroup) {
    auto childrenProperties = nodeGroup->childrenExportedProperties();

    for (auto property : childrenProperties) {
      if (!property.property->isReference()) continue;
      if (!property.property->isOutputReference()) {
        // property is input reference
        InputDot* inputDot = new vx::gui::InputDot(this);
        inputDotList << inputDot;

        inputDot->setProperty(property);
      } else {
        // property is output reference
        OutputDot* outputDot = new vx::gui::OutputDot(this);
        outputDotList << outputDot;
        outputDot->setProperty(property);
      }
    }

  } else {
    int inputCount = this->modelNode()->nodeKind() == vx::NodeKind::Data
                         ? 1
                         : getInputCount();
    for (int i = -inputCount + 1; i < inputCount; i += 2) {
      InputDot* inputDot = new vx::gui::InputDot(this);
      inputDotList << inputDot;
    }

    int counter = 0;
    for (const auto& property :
         this->modelNode()->prototype()->nodeProperties()) {
      if (!property->isReference() || property->isOutputReference()) continue;
      this->getInputDotList()[counter]->setProperty(
          NodeNodeProperty(this->modelNode(), property));
      counter++;
    }

    // TODO: This should somehow be kept in sync with Edge::Edge()
    int outputCount =
        (this->modelNode()->nodeKind() == vx::NodeKind::Data ||
         this->modelNode()->nodeKind() == vx::NodeKind::Property ||
         this->modelNode()->nodeKind() == vx::NodeKind::Object3D ||
         this->modelNode()->nodeKind() == vx::NodeKind::SegmentationStep)
            ? 1
            : getOutputCount();
    for (int i = -outputCount + 1; i < outputCount; i += 2) {
      OutputDot* outputDot = new vx::gui::OutputDot(this);
      outputDotList << outputDot;
    }

    counter = 0;
    for (const auto& property :
         this->modelNode()->prototype()->nodeProperties()) {
      if (!property->isReference() || !property->isOutputReference()) continue;
      this->getOutputDotList()[counter]->setProperty(
          NodeNodeProperty(this->modelNode(), property));
      counter++;
    }
  }
}

void GraphNode::calculateForces() { newPos = pos(); }

bool GraphNode::advanceNode() {
  if (newPos == pos()) {
    return false;
  }
  setPos(newPos);
  return true;
}

QRectF GraphNode::boundingRect() const {
  if (this->inputDotList.size() >= 5 || this->outputDotList.size() >= 5) {
    int maxDotListSize =
        std::max(this->inputDotList.size(), this->outputDotList.size());
    return QRectF(-std::ceil((maxDotListSize * 20) / 2) - 5,
                  -this->height / 2.0 - 21, maxDotListSize * 20 + 10,
                  this->height + 42);

  } else {
    return QRectF(-this->width / 2.0 - 1, -this->height / 2.0 - 21,
                  this->width + 2, this->height + 42);
  }
}

QPainterPath GraphNode::shape() const {
  QPainterPath path;
  if (this->inputDotList.size() >= 5 || this->outputDotList.size() >= 5) {
    int maxDotListSize =
        std::max(this->inputDotList.size(), this->outputDotList.size());
    path.addRect(-std::ceil((maxDotListSize * 20) / 2),
                 -this->height / 2.0 - 21, maxDotListSize * 20,
                 this->height + 42);
  } else {
    path.addRect(-this->width / 2.0 - 1, -this->height / 2.0 - 21,
                 this->width + 2, this->height + 42);
  }
  return path;
}

void GraphNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                      QWidget*) {
  (void)option;
  painter->setPen(this->graph_->edgeColor);
  painter->setBrush(Qt::transparent);
  int subPos = 0;

  QList<InputDot*> inputDotList = getInputDotList();
  if (modelNode()->nodeKind() != vx::NodeKind::Property) {
    if (inputDotList.size() < 5) {
      subPos = -inputDotList.size() + 1;
      for (vx::gui::InputDot* inputDot : inputDotList) {
        inputDot->setPos(
            QPoint(-inputDot->width / 2 + (inputDot->width * subPos),
                   -this->height / 2 - edgeOffset));
        inputDot->paint(painter);
        subPos += 2;
      }
    } else {  // handle nodes with too many inputDots
      subPos = -inputDotList.size() + 1;
      for (vx::gui::InputDot* inputDot : inputDotList) {
        inputDot->setPos(QPoint(
            -inputDot->width / 2 + ((inputDot->width + 1) * (subPos / 2)),
            -this->height / 2 - edgeOffset));
        inputDot->paint(painter);
        subPos += 2;
      }
    }
  }

  QList<OutputDot*> outputDotList = getOutputDotList();
  if (modelNode()->nodeKind() != vx::NodeKind::Visualizer) {
    if (outputDotList.size() < 5) {
      subPos = -outputDotList.size() + 1;
      for (OutputDot* outputDot : outputDotList) {
        outputDot->setPos(
            QPoint(-outputDot->width / 2 + ((outputDot->width + 1) * subPos),
                   this->height / 2));
        outputDot->paint(painter);
        subPos += 2;
      }
    } else {
      subPos = -outputDotList.size() + 1;
      for (OutputDot* outputDot : outputDotList) {
        outputDot->setPos(
            QPoint(-outputDot->width / 2 + (outputDot->width * (subPos / 2)),
                   this->height / 2));
        outputDot->paint(painter);
        subPos += 2;
      }
    }
  }

  painter->setBrush(Qt::lightGray);

  if (modelNode()->nodeKind() == vx::NodeKind::Data) {
    painter->setBrush(this->graph_->dataNodeColor);
  }
  if (modelNode()->nodeKind() == vx::NodeKind::Filter) {
    painter->setBrush(this->graph_->filterNodeColor);
  }
  if (modelNode()->nodeKind() == vx::NodeKind::Visualizer) {
    painter->setBrush(this->graph_->visualizerNodeColor);
  }
  if (modelNode()->nodeKind() == vx::NodeKind::Property) {
    painter->setBrush(this->graph_->propertyNodeColor);
  }
  if (modelNode()->nodeKind() == vx::NodeKind::Object3D) {
    painter->setBrush(this->graph_->object3DNodeColor);
  }

  // draw outline around node if it is selected
  if (this->graph_->selectedNodes().contains(this->modelNode())) {
    painter->setPen(QPen(this->graph_->edgeColor, 2));
  } else {
    painter->setPen(Qt::transparent);
  }
  painter->drawRect(-this->width / 2.0, -this->height / 2.0, this->width,
                    this->height);

  this->drawTextOnRect(painter);
}

void GraphNode::drawTextOnRect(QPainter* painter) {
  QRectF textRectAutomaticName(
      -this->width / 2.0 + this->iconPadding, -this->height / 2.0,
      this->width - this->iconPadding, this->height / 3.0);
  QRectF textRectManualName(-this->width / 2.0 + this->iconPadding,
                            (-this->height / 2.0) + this->height / 3.0,
                            this->width - this->iconPadding,
                            this->height - (this->height / 3.0));
  QFont font = painter->font();
  font.setBold(this->graph_->selectedNodes().contains(this->modelNode()));
  if (modelNode()->nodeKind() == vx::NodeKind::Property) {
    painter->setPen(Qt::white);
  } else {
    painter->setPen(Qt::black);
  }
  this->getIcon().paint(painter, -this->width / 2.0, -this->height / 2.0,
                        this->height, this->height, Qt::AlignCenter,
                        QIcon::Normal, QIcon::On);
  font.setPixelSize(6.5 / 72.0 * 96.0);
  painter->setFont(font);
  painter->drawText(textRectAutomaticName, Qt::AlignVCenter | Qt::AlignLeft,
                    this->getStaticDisplayTitle(font));
  font.setPixelSize(10 / 72.0 * 96.0);
  painter->setFont(font);
  painter->drawText(textRectManualName, Qt::AlignTop | Qt::AlignLeft,
                    this->getDisplayTitle(font));
}

QVariant GraphNode::itemChange(GraphicsItemChange change,
                               const QVariant& value) {
  switch (change) {
    case ItemPositionHasChanged:
      for (vx::gui::Edge* edge : graph_->edges()) {
        edge->adjust();
      }
      graph_->itemMoved();
      break;
    default:
      break;
  };

  return QGraphicsItem::itemChange(change, value);
}

void GraphNode::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  // qDebug() << "GraphNode::mousePressEvent";

  update();

  if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton)
    return;

  if (event->pos().y() > this->height / 2) {
    if (event->button() != Qt::LeftButton) return;

    newEdgeAction = true;
    setFlag(ItemIsMovable, false);

    if (this->graph_->hoverOutputDot != nullptr) {
      this->graph_->arrowStart = this->graph_->hoverOutputDot;
      this->graph_->myScene->clickIsHandled = true;
    }

  } else if (event->pos().x() < -this->width / 2) {
    if (event->button() != Qt::LeftButton) return;

    newEdgeAction = true;
    setFlag(ItemIsMovable, false);
  } else if (event->pos().y() < -this->height / 2) {
    if (event->button() != Qt::LeftButton) return;

    newEdgeAction = true;
    setFlag(ItemIsMovable, false);
  } else {
    this->graph_->myScene->clickIsHandled = true;
    setFlag(ItemIsMovable, true);
    // if ctrl is pressed allow selection of multiple nodes, otherwise clear
    // selectedNodes so that only the clicked node is selected
    if (!(QApplication::keyboardModifiers() &
          Qt::KeyboardModifier::ControlModifier)) {
      if (!graph_->selectedNodes().contains(modelNode_))
        graph_->setSelectedNodes({modelNode_});
    } else {
      if (!graph_->selectedNodes().contains(modelNode_)) {
        graph_->selectNode(modelNode_);
      } else {
        // Right mouse button will only add to selection
        if (event->button() != Qt::RightButton)
          graph_->deselectNode(modelNode_);
      }
    }

    newEdgeAction = false;
  }

  QGraphicsItem::mousePressEvent(event);
}

void GraphNode::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
  if (event->pos().x() > this->width / 2) {
    // Do nothing
  } else if (event->pos().x() < -this->width / 2) {
    // Do nothing
  } else {
    this->graph_->graphNodeActivated(this);
  }

  QGraphicsItem::mouseDoubleClickEvent(event);
}

void GraphNode::updateInputPos() {
  QList<InputDot*> inputDotList = this->getInputDotList();
  int subPos = -inputDotList.size() + 1;
  if (inputDotList.size() < 5) {
    for (InputDot* inputDot : inputDotList) {
      inputDot->setPos(QPoint(this->pos().x() + (inputDot->width * subPos),
                              this->pos().y() - this->height / 2 - edgeOffset));
      subPos += 2;
    }
  } else {  // handle nodes with too many inputDots
    for (InputDot* inputDot : inputDotList) {
      inputDot->setPos(
          QPoint(this->pos().x() + ((inputDot->width + 1) * (subPos / 2)),
                 this->pos().y() - this->height / 2 - edgeOffset));
      subPos += 2;
    }
  }
}

void GraphNode::updateOutputPos() {
  QList<OutputDot*> outputDotList = this->getOutputDotList();
  int subPos = -outputDotList.size() + 1;
  if (outputDotList.size() < 5) {
    for (OutputDot* outputDot : outputDotList) {
      outputDot->setPos(
          QPoint(this->pos().x() + (outputDot->width * subPos),
                 this->pos().y() + this->height / 2 + edgeOffset));
      subPos += 2;
    }
  } else {  // handle nodes with too many inputDots
    for (OutputDot* outputDot : outputDotList) {
      outputDot->setPos(
          QPoint(this->pos().x() + ((outputDot->width + 1) * (subPos / 2)),
                 this->pos().y() + this->height / 2 + edgeOffset));
      subPos += 2;
    }
  }
}

QList<QPoint> GraphNode::getInputPos() {
  QList<QPoint> inputPos;
  this->updateInputPos();
  for (InputDot* inputDot : this->getInputDotList()) {
    inputPos << inputDot->pos();
  }
  return inputPos;
}

QList<QPoint> GraphNode::getOutputPos() {
  QList<QPoint> outputPos;
  this->updateOutputPos();
  for (OutputDot* outputDot : this->getOutputDotList()) {
    outputPos << outputDot->pos();
  }
  return outputPos;
}

void GraphNode::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsItem::mouseReleaseEvent(event);
  update();
}
void GraphNode::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
  (void)event;
  update(boundingRect());
}

void GraphNode::setModelNode(Node* node) {
  auto modelNodeGroup = dynamic_cast<NodeGroup*>(modelNode_.data());
  auto newNodeGroup = dynamic_cast<NodeGroup*>(node);

  if (modelNodeGroup) {
    disconnect(modelNodeGroup, &NodeGroup::childrenExportedPropertiesChanged,
               this, &GraphNode::onChildrenExportedPropertiesChanged);
  }

  if (newNodeGroup) {
    connect(newNodeGroup, &NodeGroup::childrenExportedPropertiesChanged, this,
            &GraphNode::onChildrenExportedPropertiesChanged);
  }

  this->modelNode_ = node;
  this->title_ = modelNode_->displayName();
  QObject::connect(
      node, &Node::displayNameChanged, this,
      [&](QString displayName) {
        this->title_ = displayName;
        this->update();
      },
      Qt::UniqueConnection);
  this->populateIODots();
}

QString GraphNode::getDisplayTitle(QFont font) {
  QFontMetrics fontMetric = QFontMetrics(font);
  QString title = this->title();
  QString displayTitle = shortenStringToNodeWidth(title, font);
  return displayTitle;
}

QString GraphNode::getStaticDisplayTitle(QFont font) {
  QString title = this->modelNode()->prototype()->displayName();
  QString displayTitle = shortenStringToNodeWidth(title, font);
  return displayTitle;
}

QString GraphNode::shortenStringToNodeWidth(QString string, QFont font) {
  QString finalString = string;
  QFontMetrics fontMetric = QFontMetrics(font);
  int textWidth = fontMetric.boundingRect(finalString).width();
  while (textWidth + this->iconPadding + 10 > this->width && string != "") {
    string = string.left(string.length() - 1).trimmed();
    finalString = string + "...";
    textWidth = fontMetric.boundingRect(finalString).width();
  }

  return finalString;
}
void GraphNode::onChildrenExportedPropertiesChanged() { populateIODots(); }
