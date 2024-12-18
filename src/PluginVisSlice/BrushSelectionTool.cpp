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

#include "BrushSelectionTool.hpp"

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Node/ParameterCopy.hpp>

#include <PluginVisSlice/Prototypes.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/DebugOptions.hpp>

#include <Voxie/Vis/VisualizerView.hpp>

#include <QtGui/QGuiApplication>
#include <QtGui/QPainter>

#include <math.h>
#include <cmath>

using namespace vx;

void BrushSelectionTool::setBrushRadius(quint8 radius) {
  this->brushRadius = radius;
}

quint8 BrushSelectionTool::getBrushRadius() { return this->brushRadius; }

void BrushSelectionTool::activateTool() {
  if (!getStepManager()) return;
  valueButton->setChecked(true);

  this->stepManager->setBrushSelectionProperties(this->sv->getCuttingPlane(),
                                                 this->sv);
}

void BrushSelectionTool::deactivateTool() {
  valueButton->setChecked(false);
  this->triggerLayerRedraw(false);

  SegmentationI* segmentation =
      dynamic_cast<SegmentationI*>(this->sv->properties->segmentationFilter());

  if (segmentation) {
    segmentation->deactivateBrushes();
  }
}

void BrushSelectionTool::toolMousePressEvent(
    QMouseEvent* ev, const vx::Vector<double, 2>& pixelPos) {
  if (ev->button() == Qt::LeftButton) {
    if (vx::debug_option::Log_VisSlice_BrushSelection()->get())
      qDebug() << "Press";

    this->mousePressed = true;
    this->startPosPixel = pixelPos;
    runBrushSelection(pixelPos);
  }
}

void BrushSelectionTool::toolMouseReleaseEvent(
    QMouseEvent* ev, const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(pixelPos);

  if (ev->button() == Qt::LeftButton) {
    this->mousePressed = false;

    // TODO: Add the current position here?
  }
}

// TODO: Move to Vector.hpp?
template <typename T, size_t dim>
static T manhattanLength(const vx::Vector<T, dim>& vec) {
  using std::abs;

  T res = 0;
  for (size_t i = 0; i < dim; i++) res += abs(vec[i]);
  return res;
}

void BrushSelectionTool::toolMouseMoveEvent(
    QMouseEvent* ev, const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(ev);

  if (!this->mousePressed)
    return;

  auto distance = manhattanLength(pixelPos - this->startPosPixel);
  // if (distance >= 1) {
  if (distance >=
      vx::debug_option::VisSlice_BrushSelection_MinDistance()->get()) {
    if (vx::debug_option::Log_VisSlice_BrushSelection()->get())
      qDebug() << "runBrushSelection()" << distance;
    runBrushSelection(pixelPos);
    this->startPosPixel = pixelPos;
  } else {
    if (vx::debug_option::Log_VisSlice_BrushSelection()->get())
      qDebug() << "Skip mouse move" << distance;
  }
}

void BrushSelectionTool::toolLeaveEvent(QEvent* ev) {
  Q_UNUSED(ev);
  this->triggerLayerRedraw(false);
}

// currently not in use
void BrushSelectionTool::toolKeyPressEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Shift) {
  }
}

// currently not in use
void BrushSelectionTool::toolKeyReleaseEvent(QKeyEvent* e) {
  if (e->key() == Qt::Key_Shift) {
  }
}

void BrushSelectionTool::triggerLayerRedraw(bool isMouseValid) {
  const auto layer =
      std::find_if(this->sv->layers().begin(), this->sv->layers().end(),
                   [this](const QSharedPointer<Layer> l) {
                     return l->objectName() == this->sv->brushLayerName;
                   });

  if (layer != this->sv->layers().end()) {
    layer->data()->triggerRedraw();
    ((BrushSelectionLayer*)layer->data())->mousePosValid = isMouseValid;
  }
}

bool BrushSelectionTool::getStepManager() {
  SegmentationI* segmentation =
      dynamic_cast<SegmentationI*>(this->sv->properties->segmentationFilter());

  if (segmentation) {
    this->stepManager = segmentation->getStepManager();
    return true;
  } else {
    return false;
  }
}

void BrushSelectionTool::runBrushSelection(
    const vx::Vector<double, 2>& middlePoint) {
  auto pos3D = this->sv->pixelPosTo3DPosCurrentImage(middlePoint);

  // TODO: Add a meter calculation function
  double brushRadiusMeter = std::sqrt(squaredNorm(
      this->sv->pixelPosToPlanePosCurrentImage({(double)this->brushRadius, 0}) -
      this->sv->pixelPosToPlanePosCurrentImage({0, 0})));

  this->stepManager->addVoxelsToBrushSelection(
      std::tuple<vx::Vector<double, 3>, double>(pos3D, brushRadiusMeter),
      sv->getCuttingPlane(), this->sv);
}

BrushSelectionTool::BrushSelectionTool(QWidget* parent, SliceVisualizer* sv)
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
}

BrushSelectionLayer::BrushSelectionLayer(SliceVisualizer* sv) : sv(sv) {
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

  // TODO: Why are these connected to triggerRedraw?
  connect(sv, &SliceVisualizer::gpoDataChangedFinished, this,
          &Layer::triggerRedraw);
  connect(sv, &SliceVisualizer::currentPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv, &SliceVisualizer::newMeasurement, this, &Layer::triggerRedraw);

  // Redraw when colors change
  QObject::connect(this->sv->properties,
                   &SliceProperties::geometricPrimitiveColorOnSliceChanged,
                   this, &BrushSelectionLayer::triggerRedraw);
  QObject::connect(
      this->sv->properties,
      &SliceProperties::geometricPrimitiveColorInFrontOfSliceChanged, this,
      &BrushSelectionLayer::triggerRedraw);
  QObject::connect(this->sv->properties,
                   &SliceProperties::geometricPrimitiveColorBehindSliceChanged,
                   this, &BrushSelectionLayer::triggerRedraw);

  QObject::connect(
      sv, &SliceVisualizer::imageMouseMove, this,
      [this, sv](QMouseEvent* e, const vx::Vector<double, 2>& planePos,
                 const vx::Vector<double, 3>& pos3D,
                 const vx::Vector<double, 3>* posVoxelPtr, double valNearest,
                 double valLinear) {
        Q_UNUSED(planePos);
        Q_UNUSED(posVoxelPtr);
        Q_UNUSED(valNearest);
        Q_UNUSED(valLinear);

        SegmentationI* segmentation = dynamic_cast<SegmentationI*>(
            this->sv->properties->segmentationFilter());
        if (segmentation) {
          // TODO: Don't use QVector3D
          segmentation->updatePosition(
              toQVector(vectorCastNarrow<float>(pos3D)));
        }

        this->mousePos = e->pos();
        this->mousePosValid = true;

        if (sv->currentTool()->objectName() == "BrushSelectionTool")
          this->triggerRedraw();
      });
}

void BrushSelectionLayer::drawCircle(QImage& outputImage, float visibility,
                                     QPointF point, int radius) {
  QPainter painter(&outputImage);
  QPen pen;
  pen.setStyle(Qt::SolidLine);
  pen.setWidth(2);
  QColor color("Blue");
  color.setAlphaF(visibility);
  pen.setColor(color);
  painter.setPen(pen);
  QFont font;  // applications default
  painter.setFont(font);
  painter.drawEllipse(point.x() - radius, point.y() - radius, 2 * radius,
                      2 * radius);
}

void BrushSelectionLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters,
    bool isMainImage) {
  Q_UNUSED(isMainImage);

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (sv->currentTool()->objectName() == "BrushSelectionTool" &&
      this->mousePosValid) {
    this->drawCircle(outputImage, 1.0, this->mousePos, this->brushRadius);
  }

  auto gpoPath = properties.geometricPrimitiveRaw();
  if (gpoPath == QDBusObjectPath("/")) return;
  GeometricPrimitivePropertiesCopy gpoProperties(
      parameters->properties()[gpoPath]);
}

void BrushSelectionLayer::setBrushRadius(quint8 radius) {
  this->brushRadius = radius;
}

quint8 BrushSelectionLayer::getBrushRadius() { return this->brushRadius; }
