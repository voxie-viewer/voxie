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

#include "Edge.hpp"
#include "GraphWidget.hpp"
#include "Node.hpp"

#include <qmath.h>
#include <QPainter>

#include <Voxie/Node/Types.hpp>

using namespace vx::gui;

Edge::Edge(OutputDot* sourceDot, InputDot* destDot)
    : Edge(sourceDot->graphNode(), sourceDot, destDot->graphNode(), destDot) {}

Edge::Edge(GraphNode* sourceNode, OutputDot* sourceDot, GraphNode* destNode,
           InputDot* destDot)
    : arrowSize(10) {
  setAcceptedMouseButtons(Qt::MouseButtons());

  this->sourceDot = sourceDot;
  this->destDot = destDot;

  this->source = sourceNode;
  this->dest = destNode;

  adjust();
}

GraphNode* Edge::sourceNode() const { return source; }

GraphNode* Edge::destNode() const { return dest; }

void Edge::adjust() {
  if (!source || !dest) return;

  source->updateOutputPos();
  dest->updateInputPos();

  QPointF from = mapFromItem(source, 0, 0);
  QPointF to = mapFromItem(dest, 0, 0);

  if (!this->sourceDot) {
    qWarning() << "Edge::adjust: sourceDot is nullptr";
    return;
  }
  if (!this->destDot) {
    qWarning() << "Edge::adjust: destDot is nullptr";
    return;
  }

  from = this->sourceDot->pos();
  to = this->destDot->pos();

  QLineF line(from, to);
  qreal length = line.length();

  prepareGeometryChange();

  QPointF edgeOffset((line.dx() * 2) / length, (line.dy() * 2) / length);
  if (length == 0)
    // TODO: What should happen in this case? What should happen for length ~ 0?
    edgeOffset = QPointF(0, 2);
  sourcePoint = line.p1() + edgeOffset;
  destPoint = line.p2() - edgeOffset;
}

QRectF Edge::boundingRect() const {
  if (!source || !dest) return QRectF();

  qreal penWidth = 1;
  qreal extra = (penWidth + arrowSize) / 2.0;

  return QRectF(sourcePoint, QSizeF(destPoint.x() - sourcePoint.x(),
                                    destPoint.y() - sourcePoint.y()))
      .normalized()
      .adjusted(-extra, -extra, extra, extra);
}

void Edge::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
  if (!source || !dest || !visible) return;

  QLineF line(sourcePoint, destPoint);
  if (qFuzzyCompare(line.length(), qreal(0.))) return;

  // Draw the line itself
  painter->setPen(QPen(source->graph()->edgeColor, 1, Qt::SolidLine,
                       Qt::RoundCap, Qt::RoundJoin));
  painter->drawLine(line);

  // Draw the arrows
  double angle = std::atan2(-line.dy(), line.dx());

  QPointF destArrowP1 = destPoint + QPointF(sin(angle - M_PI / 3) * arrowSize,
                                            cos(angle - M_PI / 3) * arrowSize);
  QPointF destArrowP2 =
      destPoint + QPointF(sin(angle - M_PI + M_PI / 3) * arrowSize,
                          cos(angle - M_PI + M_PI / 3) * arrowSize);

  painter->setBrush(source->graph()->edgeColor);
  painter->drawPolygon(QPolygonF() << line.p2() << destArrowP1 << destArrowP2);
}
