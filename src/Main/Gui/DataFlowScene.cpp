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

#include "DataFlowScene.hpp"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QScrollBar>
#include <Voxie/Node/Types.hpp>
#include "Node.hpp"

using namespace vx::gui;

DataFlowScene::DataFlowScene(GraphWidget* graphWidget)
    : QGraphicsScene(graphWidget) {
  this->graphWidget = graphWidget;
  connect(graphWidget, &GraphWidget::mousePosChanged, this,
          &DataFlowScene::onMouseChanged);
}

void DataFlowScene::drawForeground(QPainter* painter, const QRectF& rect) {
  (void)rect;
  if (graphWidget->arrowStart != nullptr) {
    graphWidget->arrowStart->graphNode()->updateOutputPos();
    QColor hoverColor;

    // snap arrow to input dots on hover
    QLineF line(graphWidget->arrowStart->pos(), m_MousePos);
    if (graphWidget->hoverNode != nullptr &&
        graphWidget->arrowStart->graphNode() != graphWidget->hoverNode) {
      InputDot* snapToDot = nullptr;
      bool multiplePossibleInputs = false;
      for (InputDot* inputDot : graphWidget->hoverNode->getInputDotList()) {
        if (abs(inputDot->pos().x() - m_MousePos.x()) < 10 &&
            abs(inputDot->pos().y() - m_MousePos.y()) < 20) {
          snapToDot = inputDot;
          break;
        } else if (inputDot->property().property &&
                   inputDot->property().property->allowsAsValue(
                       graphWidget->arrowStart->propertyOrModelNode()
                           ->prototype())) {
          if (!multiplePossibleInputs) {
            snapToDot = inputDot;
            multiplePossibleInputs = true;
          } else {
            snapToDot = nullptr;
          }
        }
      }
      if (snapToDot != nullptr)
        line.setP2(snapToDot->pos());
      else
        line.setP2(graphWidget->hoverNode->pos());

      // color input dot and arrow depending on currently drawn connection
      painter->setOpacity(0.25);
      if (graphWidget->hoverInputDot != nullptr) {
        if (graphWidget->hoverInputDot->property().property) {
          if (graphWidget->hoverInputDot->property().property->allowsAsValue(
                  graphWidget->arrowStart->propertyOrModelNode())) {
            if (graphWidget->hoverInputDot->property().property->type() !=
                    vx::types::NodeReferenceListType() &&
                graphWidget->hoverInputDot->getPropertyAsObject() ==
                    graphWidget->arrowStart->propertyOrModelNode())
              hoverColor = graphWidget->inputDotAlreadyConnectedColor;
            else if (graphWidget->hoverInputDot->property().property->type() ==
                         vx::types::NodeReferenceListType() &&
                     graphWidget->hoverInputDot->getPropertyAsObjectList()
                         .contains(
                             graphWidget->arrowStart->propertyOrModelNode()))
              hoverColor = graphWidget->inputDotAlreadyConnectedColor;
            else
              hoverColor = graphWidget->inputDotValidColor;

          } else {
            hoverColor = graphWidget->inputDotInvalidColor;
          }
        } else {
          hoverColor = graphWidget->inputDotUnverifiedColor;
        }
        painter->fillRect(
            QRect(QPoint(graphWidget->hoverInputDot->pos().x() -
                             (graphWidget->hoverInputDot->width / 2),
                         graphWidget->hoverInputDot->pos().y()),
                  QPoint(graphWidget->hoverInputDot->pos().x() +
                             (graphWidget->hoverInputDot->width / 2),
                         graphWidget->hoverInputDot->pos().y() +
                             (graphWidget->hoverInputDot->height - 1))),
            hoverColor);

      } else if (graphWidget->hoverNode != nullptr) {
        InputDot* targetInputDot = nullptr;
        for (InputDot* inputDot : graphWidget->hoverNode->getInputDotList()) {
          if (inputDot->property().property &&
              inputDot->property().property->allowsAsValue(
                  graphWidget->arrowStart->propertyOrModelNode()
                      ->prototype())) {
            if (targetInputDot == nullptr) {
              targetInputDot = inputDot;
            } else {
              targetInputDot = nullptr;
              break;
            }
          }
        }
        if (targetInputDot != nullptr) {
          if (targetInputDot->property().property->type() !=
                  vx::types::NodeReferenceListType() &&
              targetInputDot->getPropertyAsObject() ==
                  graphWidget->arrowStart->propertyOrModelNode())
            hoverColor = graphWidget->inputDotAlreadyConnectedColor;
          else if (targetInputDot->property().property->type() ==
                       vx::types::NodeReferenceListType() &&
                   targetInputDot->getPropertyAsObjectList().contains(
                       graphWidget->arrowStart->propertyOrModelNode()))
            hoverColor = graphWidget->inputDotAlreadyConnectedColor;
          else
            hoverColor = graphWidget->inputDotValidColor;
          painter->fillRect(QRect(QPoint(targetInputDot->pos().x() -
                                             (targetInputDot->width / 2),
                                         targetInputDot->pos().y()),
                                  QPoint(targetInputDot->pos().x() +
                                             (targetInputDot->width / 2),
                                         targetInputDot->pos().y() +
                                             (targetInputDot->height - 1))),
                            hoverColor);
        }
      }
      painter->setOpacity(1);
    }

    // Draw the line itself
    painter->setPen(QPen(graphWidget->edgeColor, 1, Qt::SolidLine, Qt::RoundCap,
                         Qt::RoundJoin));

    if (hoverColor != nullptr) painter->setPen(hoverColor);

    painter->drawLine(line);

    // Draw the arrows
    double angle = std::atan2(-line.dy(), line.dx());
    qreal arrowSize = 10;
    QPointF destArrowP1 =
        line.p2() + QPointF(sin(angle - M_PI / 3) * arrowSize,
                            cos(angle - M_PI / 3) * arrowSize);
    QPointF destArrowP2 =
        line.p2() + QPointF(sin(angle - M_PI + M_PI / 3) * arrowSize,
                            cos(angle - M_PI + M_PI / 3) * arrowSize);
    painter->setBrush(graphWidget->edgeColor);
    painter->drawPolygon(QPolygonF()
                         << line.p2() << destArrowP1 << destArrowP2);
  }
}

void DataFlowScene::onMouseChanged(QPoint newMousePos) {
  Q_UNUSED(newMousePos);
}

void DataFlowScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
  QGraphicsScene::mouseReleaseEvent(event);

  this->graphWidget->setDragMode(QGraphicsView::NoDrag);

  if (event->button() == Qt::RightButton) {
    graphWidget->requestContextMenu(
        event->buttonDownScreenPos(Qt::RightButton));
  } else {
    // auto item = itemAt(event->scenePos(), QTransform()); // TODO?

    if (graphWidget->arrowStart != nullptr) {
      if (graphWidget->hoverInputDot != nullptr) {
        graphWidget->connectNodes(graphWidget->arrowStart,
                                  graphWidget->hoverInputDot);

      } else if (graphWidget->hoverNode != nullptr &&
                 graphWidget->hoverNode->modelNode() !=
                     graphWidget->arrowStart->propertyOrModelNode()) {
        graphWidget->connectNodes(graphWidget->arrowStart,
                                  graphWidget->hoverNode);
      }
    }
    graphWidget->arrowStart = nullptr;
  }
  invalidate();
}

void DataFlowScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
  m_MousePos = event->scenePos().toPoint();
  GraphNode* newHoverNode = nullptr;
  InputDot* newHoverInputDot = nullptr;
  OutputDot* newHoverOutputDot = nullptr;
  auto item = itemAt(m_MousePos, QTransform());
  if (item != nullptr) {
    if (GraphNode* node = dynamic_cast<GraphNode*>(item)) {
      newHoverNode = node;
      QList<InputDot*> inputDotList = node->getInputDotList();
      for (int i = 0; i < inputDotList.size(); i++) {
        if (abs(node->getInputPos()[i].x() - m_MousePos.x()) < 10 &&
            abs(node->getInputPos()[i].y() - m_MousePos.y()) < 20) {
          newHoverInputDot = node->getInputDotList()[i];
        }
      }
      QList<OutputDot*> outputDotList = node->getOutputDotList();
      for (int i = 0; i < outputDotList.size(); i++) {
        if (abs(node->getOutputPos()[i].x() - m_MousePos.x()) < 10 &&
            abs(node->getOutputPos()[i].y() - m_MousePos.y()) < 20) {
          newHoverOutputDot = node->getOutputDotList()[i];
        }
      }
    }
  }
  this->graphWidget->hoverNode = newHoverNode;
  this->graphWidget->hoverInputDot = newHoverInputDot;
  this->graphWidget->hoverOutputDot = newHoverOutputDot;

  // Tells the scene it should be redrawn
  invalidate();

  QGraphicsScene::mouseMoveEvent(event);
}

void DataFlowScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
  if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
    this->graphWidget->lastClickPos = QPointF();
    auto item = itemAt(event->scenePos(), QTransform());
    if (item == nullptr || !dynamic_cast<GraphNode*>(item)) {
      if (event->button() == Qt::RightButton) {
        this->graphWidget->lastClickPos = event->scenePos();
      } else {
        this->graphWidget->setDragMode(QGraphicsView::ScrollHandDrag);
      }
    } else {
      this->dragStart = QPointF();
    }
  }

  QGraphicsScene::mousePressEvent(event);
}
