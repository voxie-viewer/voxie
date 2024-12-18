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
  connect(button, &QPushButton::clicked, [=]() { sv->switchToolTo(this); });
  // zoomButton->setUpdatesEnabled(false);
  layout->addWidget(button, 0, 0);
  button->show();
  this->setLayout(layout);
}

void DefaultTool::activateTool() { button->setChecked(true); }

void DefaultTool::deactivateTool() { button->setChecked(false); }

void DefaultTool::toolWheelEvent(QWheelEvent* ev,
                                 const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(ev);
  Q_UNUSED(pixelPos);
}

void DefaultTool::toolMousePressEvent(QMouseEvent* ev,
                                      const vx::Vector<double, 2>& pixelPos) {
  if (ev->button() != Qt::LeftButton) return;

  if ((ev->buttons() & (Qt::MiddleButton | Qt::RightButton)) != Qt::NoButton)
    return;

  auto modifiers = ev->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

  if (modifiers == Qt::ShiftModifier &&
      this->sv->properties->showSliceCenter()) {
    // set origin to cursor
    auto planePos = this->sv->pixelPosToPlanePosCurrentImage(pixelPos);
    // this->sv->properties->setCenterPoint(planePos);
    this->sv->movePlaneOrigin(planePos);
  } else if (modifiers == Qt::ControlModifier) {
    this->savePoint(pixelPos);
  }
}

void DefaultTool::toolMouseMoveEvent(QMouseEvent* ev,
                                     const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(ev);
  Q_UNUSED(pixelPos);

  // TODO: Support adding lines by dragging the mouse?
  /*
  if (this->mousePressed &&
      (ev->pos() - this->startPos).manhattanLength() > 2) {
  } else {
  }
  */
}

void DefaultTool::toolMouseReleaseEvent(QMouseEvent* ev,
                                        const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(ev);
  Q_UNUSED(pixelPos);
}

void DefaultTool::toolKeyPressEvent(QKeyEvent* ev) { Q_UNUSED(ev); }

void DefaultTool::toolKeyReleaseEvent(QKeyEvent* ev) { Q_UNUSED(ev); }

void DefaultTool::savePoint(const vx::Vector<double, 2>& pixelPos) {
  auto pos3D = this->sv->pixelPosTo3DPosCurrentImage(pixelPos);
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
      gpo->nextPointName(), pos3D);
  {
    auto update = gpd->createUpdate();
    gpd->addPrimitive(update, primitive);
    update->finish(QJsonObject());
  }
}
