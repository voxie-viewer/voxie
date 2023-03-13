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
#include "GraphWidget.hpp"

/**
 * @brief The DataFlowScene class is an own implementation of QGraphicsScene
 * It is used for catching basic mouse events in the dataflow graph and for
 * drawing dynamic elements as the draggable connection arrow
 */
class DataFlowScene : public QGraphicsScene {
  QPoint m_MousePos;
  vx::gui::GraphWidget* graphWidget;

  QPointF dragStart = QPointF();

 public:
  DataFlowScene(vx::gui::GraphWidget* graphWidget);

  void drawForeground(QPainter* painter, const QRectF& rect) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

  /**
   * @brief onMouseChanged callback for position change of mouse
   * @param NewMousePos
   */
  void onMouseChanged(QPoint NewMousePos);

  // TODO: Do this differently?
  bool clickIsHandled = false;

  void helpEvent(QGraphicsSceneHelpEvent* helpEvent) override;
};
