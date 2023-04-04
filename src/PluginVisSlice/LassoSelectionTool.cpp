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

#include "LassoSelectionTool.hpp"

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <PluginVisSlice/Prototypes.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

using namespace vx;

// TODO: Seems like sometimes the color of the points is not properly updated

LassoSelectionTool::LassoSelectionTool(QWidget* parent, SliceVisualizer* sv)
    : Visualizer2DTool(parent), sv(sv) {
  QGridLayout* layout = new QGridLayout(this);
  valueButton = new QPushButton(getIcon(), getName());
  valueButton->setCheckable(true);
  connect(valueButton, &QPushButton::clicked,
          [=]() { sv->switchToolTo(this); });
  // zoomButton->setUpdatesEnabled(false);
  layout->addWidget(valueButton, 0, 0);
  valueButton->show();
  this->setLayout(layout);

  // Reset the lasso selection when the SliceVisualizer plane changes
  connect(this->sv, &SliceVisualizer::rotationChangedForward, this,
          [=]() { this->reset(); });
  connect(this->sv, &SliceVisualizer::originChangedForward, this,
          [=]() { this->reset(); });

  // get lasso layer
  auto layer =
      std::find_if(this->sv->layers().begin(), this->sv->layers().end(),
                   [this](const QSharedPointer<Layer> l) {
                     return l->objectName() == this->sv->lassoLayerName;
                   });

  if (layer != this->sv->layers().end()) {
    this->lassoLayer = dynamic_cast<LassoSelectionLayer*>(layer->data());
  }

  if (!this->lassoLayer) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Visualizer.Slice.LassoSelectionTool",
        QString("No LassoLayerAvailable"));
  }
}

// TODO: Use alpha of color values?

void LassoSelectionTool::activateTool() { valueButton->setChecked(true); }

void LassoSelectionTool::deactivateTool() {
  valueButton->setChecked(false);
  this->reset();

  SegmentationI* segmentation =
      dynamic_cast<SegmentationI*>(this->sv->properties->segmentationFilter());

  if (segmentation) {
    segmentation->deactivateLasso();
  }
}

void LassoSelectionTool::reset() {
  this->lassoLayer->setNodes({});
  this->nodes = {};
  this->lines = {};
  this->triggerLayerRedraw();
}

void LassoSelectionTool::toolMousePressEvent(QMouseEvent* ev) {
  if (ev->button() == Qt::LeftButton) {
    this->mousePressed = true;
    this->startPos = ev->pos();
  } else if (ev->button() == Qt::RightButton) {
    toolMouseMoveEvent(ev);
  }
}

void LassoSelectionTool::toolMouseReleaseEvent(QMouseEvent* ev) {
  if (ev->button() == Qt::LeftButton) {
    this->mousePressed = false;
    if (this->doesPointClosePolygon(ev->pos())) {
      // Get the nodes that span the Polygon
      QPointF closestPoint = this->getClosestNode(ev->pos());
      QList<QPointF> polygonNodes =
          this->nodes.mid(this->nodes.lastIndexOf(closestPoint));

      if (!getStepManager()) return;

      // make sure that that the polygon is 2D and not 0/1D.
      if (polygonNodes.size() > 2) {
        // to close the polygon in the layer & redraw
        this->savePoint(closestPoint);
        this->triggerLayerRedraw();

        QList<QVector3D> nodes3D;
        for (auto node : polygonNodes) {
          QPoint yetAnotherKindOfPoint((int)node.x(), (int)node.y());
          QPointF planepoint = this->sv->sliceImage().pixelToPlanePoint(
              yetAnotherKindOfPoint, true);
          QVector3D threeDPoint = sv->slice()->getCuttingPlane().get3DPoint(
              planepoint.x(), planepoint.y());
          nodes3D.append(threeDPoint);
        }

        this->stepManager->setLassoSelection(
            nodes3D, this->sv->slice()->getCuttingPlane(), this->sv);

      } else {
        vx::showErrorMessage(
            "Error during LassoSelection",
            "Polygon must be a 2D surface not a line or point!");
      }
      // Reset everything (To make next Polygon drawable)
      this->reset();

    } else {
      // Check if line to new point intersects with old lines
      if (this->nodes.size() > 0) {
        QLineF newLine = QLineF(this->nodes.last(), ev->pos());

        if (this->doesNewLineIntersect(newLine)) {
          vx::showErrorMessage("Error during LassoSelection",
                               "Lines should not intersect!");

          return;
        } else {
          // Does not intersect
          this->lines.append(newLine);
        }
      }
      this->savePoint(ev->pos());
      this->triggerLayerRedraw();
    }
  }
}

void LassoSelectionTool::triggerLayerRedraw() {
  if (this->lassoLayer) {
    this->lassoLayer->triggerRedraw();
  }
}

void LassoSelectionTool::toolMouseMoveEvent(QMouseEvent* ev) {
  if (this->mousePressed &&
      (ev->pos() - this->startPos).manhattanLength() > 2) {
  } else {
  }
}

void LassoSelectionTool::savePoint(QPointF point) {
  if (!this->sv->slice()) {
    return;
  }

  // save in nodes
  this->nodes.append(point);
  // save in Layer
  this->lassoLayer->addNode(point);
}

bool LassoSelectionTool::doesPointClosePolygon(QPointF point) {
  for (auto node : this->nodes) {
    if (sqrt(pow(node.x() - point.x(), 2) + pow(node.y() - point.y(), 2)) <
        this->minDistance) {
      return true;
    }
  }

  return false;
}

QPointF LassoSelectionTool::getClosestNode(QPointF point) {
  float minDistance = std::numeric_limits<float>::max();
  QPointF closestPoint;

  for (auto node : this->nodes) {
    float distance =
        sqrt(pow(node.x() - point.x(), 2) + pow(node.y() - point.y(), 2));

    if (distance < minDistance) {
      minDistance = distance;
      closestPoint = node;
    }
  }

  return closestPoint;
}

bool LassoSelectionTool::doesNewLineIntersect(QLineF newLine) {
  for (int i = 0; i < this->lines.size() - 1; i++) {
    QLineF::IntersectType intersectionType =
        newLine.intersect(this->lines[i], new QPointF(0, 0));

    if (intersectionType == QLineF::BoundedIntersection) {
      return true;
    }
  }

  return false;
}

bool LassoSelectionTool::getStepManager() {
  SegmentationI* segmentation =
      dynamic_cast<SegmentationI*>(this->sv->properties->segmentationFilter());

  if (segmentation) {
    this->stepManager = segmentation->getStepManager();
    return true;
  } else {
    return false;
  }
}

// currently not in use
void LassoSelectionTool::toolKeyPressEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Shift) {
    // updateValueAtMouse(true);
  }
}

// currently not in use
void LassoSelectionTool::toolKeyReleaseEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Shift) {
    // updateValueAtMouse(false);
  }
}

LassoSelectionLayer::LassoSelectionLayer(SliceVisualizer* sv) : sv(sv) {
  this->visibility = 1;

  // Redraw when the slice changes
  QObject::connect(sv->properties, &SliceProperties::orientationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::originChanged, this,
                   &Layer::triggerRedraw);

  // Redraw when shown part of slice changes
  connect(sv->properties, &SliceProperties::centerPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv->properties, &SliceProperties::verticalSizeChanged, this,
          &Layer::triggerRedraw);
}

void LassoSelectionLayer::addNode(QPointF node) { this->nodes.append(node); }

void LassoSelectionLayer::setNodes(QList<QPointF> nodes) {
  this->nodes = nodes;
}

void LassoSelectionLayer::drawPoint(QImage& outputImage, float visibility,
                                    QPointF point) {
  QPainter painter(&outputImage);
  QPen pen;
  pen.setStyle(Qt::SolidLine);
  QColor color(90, 185, 234);
  color.setAlphaF(visibility);
  pen.setColor(color);
  pen.setWidth(2);
  painter.setPen(pen);
  QFont font;  // applications default
  painter.setFont(font);
  QPoint upperleft(point.x() - 5, point.y() - 5);
  QPoint lowerright(point.x() + 5, point.y() + 5);
  QPoint upperright(point.x() + 5, point.y() - 5);
  QPoint lowerleft(point.x() - 5, point.y() + 5);
  painter.setCompositionMode(QPainter::RasterOp_SourceAndNotDestination);
  painter.drawLine(upperleft, lowerright);
  painter.drawLine(upperright, lowerleft);
}

void LassoSelectionLayer::redraw(QImage& outputImage, float visibility) {
  for (int i = 0; i < this->nodes.size(); i++) {
    this->drawPoint(outputImage, visibility, this->nodes[i]);
    // DrawLine
    if (i > 0) {
      this->drawLine(outputImage, visibility, this->nodes[i],
                     this->nodes[i - 1]);
    }
  }
}

void LassoSelectionLayer::drawLine(QImage& outputImage, float visibility,
                                   QPointF p1, QPointF p2) {
  QPainter painter(&outputImage);

  QPen pen;
  QColor color(90, 185, 234);
  color.setAlphaF(visibility);
  pen.setColor(color);
  pen.setWidthF(1.5);
  painter.setPen(pen);
  painter.drawLine(p1, p2);
}

void LassoSelectionLayer::newVisibility(float newVis) {
  this->visibility = newVis;
  triggerRedraw();
}

void LassoSelectionLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters) {
  Q_UNUSED(parameters);
  // TODO: This is not multithreading safe, clean up
  auto visibility = this->visibility;

  this->redraw(outputImage, visibility);

  // if (std::get<0>(measurement))
  //   this->drawLine(outputImage, &properties, visibility,
  //                  std::get<1>(measurement), std::get<2>(measurement));
}
