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

#include <Main/Gui/GraphNode.hpp>

namespace vx {
namespace gui {

/**
 * @brief The Edge class is the graphical representation of an edge in the
 * dataflow graph
 */
class Edge : public QGraphicsItem {
 public:
  Edge(GraphNode* sourceNode, OutputDot* sourceDot, GraphNode* destNode,
       InputDot* destDot);
  Edge(OutputDot* sourceDot, InputDot* destDot);

  /**
   * @brief sourceNode source node from where the edge points
   * @return
   */
  GraphNode* sourceNode() const;
  /**
   * @brief destNode destination node to which the edge points
   * @return
   */
  GraphNode* destNode() const;

  /**
   * @brief sourceDot OutputDot at which edge starts
   * @return
   */
  OutputDot* getSourceDot() { return this->sourceDot; }

  /**
   * @brief destDot InputDot at which edge ends
   * @return
   */
  InputDot* getDestDot() { return this->destDot; }

  /**
   * @brief adjust recalculates the positions relevant to the arrow
   */
  void adjust();

  bool isVisible() { return visible; }

  void setVisible(bool value) { visible = value; }

  enum { Type = UserType + 2 };
  int type() const override { return Type; }

 protected:
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) override;

 private:
  GraphNode* source;
  GraphNode* dest;
  OutputDot* sourceDot = nullptr;
  InputDot* destDot = nullptr;
  QPointF sourcePoint;
  QPointF destPoint;
  qreal arrowSize;
  bool visible;
};
}  // namespace gui
}  // namespace vx
