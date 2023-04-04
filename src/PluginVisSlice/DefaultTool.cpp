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

#include "DefaultTool.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Data/GeometricPrimitiveObject.hpp>

#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <QtCore/QtMath>

using namespace vx;

DefaultTool::DefaultTool(QWidget* parent, SliceVisualizer* sv)
    : Visualizer2DTool(parent), sv(sv) {
  QGridLayout* layout = new QGridLayout(this);
  button = new QPushButton(getIcon(), getName());
  button->setCheckable(true);
  connect(button, &QPushButton::clicked,
          [=]() { sv->switchToolTo(this); });
  // zoomButton->setUpdatesEnabled(false);
  layout->addWidget(button, 0, 0);
  button->show();
  this->setLayout(layout);
}

void DefaultTool::activateTool() {
  button->setChecked(true);
}

void DefaultTool::deactivateTool() {
  button->setChecked(false);
}

void DefaultTool::toolWheelEvent(QWheelEvent* ev) { Q_UNUSED(ev); }

void DefaultTool::toolMousePressEvent(QMouseEvent* ev) {
  if (!this->sv->dataSet()) return;

  if (ev->button() != Qt::LeftButton) return;

  if ((ev->buttons() & (Qt::MiddleButton | Qt::RightButton)) != Qt::NoButton)
    return;

  auto modifiers =
      ev->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

  if (modifiers == Qt::ShiftModifier &&
      this->sv->properties->showSliceCenter()) {
    // set origin to cursor
    QPointF cursorOnPlane =
        this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true);
    this->sv->slice()->setOrigin(cursorOnPlane);
  } else if (modifiers == Qt::ControlModifier) {
    this->savePoint(ev->pos());
  }
}

void DefaultTool::toolMouseMoveEvent(QMouseEvent* ev) {
  Q_UNUSED(ev);

  // TODO: Support adding lines by dragging the mouse?
  /*
  if (this->mousePressed &&
      (ev->pos() - this->startPos).manhattanLength() > 2) {
  } else {
  }
  */
}

void DefaultTool::toolMouseReleaseEvent(QMouseEvent* ev) { Q_UNUSED(ev); }

void DefaultTool::toolKeyPressEvent(QKeyEvent* ev) { Q_UNUSED(ev); }

void DefaultTool::toolKeyReleaseEvent(QKeyEvent* ev) { Q_UNUSED(ev); }

void DefaultTool::savePoint(QPointF point) {
  if (!this->sv->slice()) {
    return;
  }
  vx::SliceImage sliceImg =
      (false ? this->sv->filteredSliceImage() : this->sv->sliceImage());
  QPoint yetAnotherKindOfPoint((int)point.x(), (int)point.y());
  QPointF planepoint = sliceImg.pixelToPlanePoint(yetAnotherKindOfPoint, true);
  QVector3D threeDPoint =
      sv->slice()->getCuttingPlane().get3DPoint(planepoint.x(), planepoint.y());
  auto gpo = dynamic_cast<GeometricPrimitiveNode*>(
      this->sv->properties->geometricPrimitive());
  if (!gpo) {
    gpo = dynamic_cast<GeometricPrimitiveNode*>(
        GeometricPrimitiveNode::getPrototypeSingleton()
            ->create(QMap<QString, QVariant>(), QList<Node*>(),
                     QMap<QString, QDBusVariant>())
            .data());
    if (!gpo) {
      qWarning() << "Failed to create GeometricPrimitiveNode";
      return;
    }
    this->sv->properties->setGeometricPrimitive(gpo);
  }
  auto gpd = gpo->geometricPrimitiveData();
  if (!gpd) {
    gpd = GeometricPrimitiveData::create();
    gpo->setGeometricPrimitiveData(gpd);
  }
  auto primitive = createQSharedPointer<GeometricPrimitivePoint>(
      gpo->nextPointName(), threeDPoint);
  {
    auto update = gpd->createUpdate();
    gpd->addPrimitive(update, primitive);
    update->finish(QJsonObject());
  }
}
