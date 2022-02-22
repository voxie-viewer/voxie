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

#include "SliceAdjustmentTool.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

#include <QtCore/QtMath>

using namespace vx;

SliceAdjustmentTool::SliceAdjustmentTool(QWidget* parent, SliceVisualizer* sv)
    : Visualizer2DTool(parent), sv(sv), shiftDown(false) {
  QGridLayout* layout = new QGridLayout(this);
  adjustButton = new QPushButton(getIcon(), getName());
  adjustButton->setCheckable(true);
  connect(adjustButton, &QPushButton::clicked,
          [=]() { sv->switchToolTo(this); });
  // zoomButton->setUpdatesEnabled(false);
  layout->addWidget(adjustButton, 0, 0);
  adjustButton->show();
  this->setLayout(layout);
}

void SliceAdjustmentTool::activateTool() {
  // qDebug() << "activate adjust";
  this->isActive = true;
  adjustButton->setChecked(true);

  // this->draw();
}

void SliceAdjustmentTool::deactivateTool() {
  // qDebug() << "deactivate adjust";
  this->isActive = false;
  adjustButton->setChecked(false);

  /*
  sv->addToDrawStack(
      this, QImage());  // temporary alternative to Q_EMIT sliceImageChanged
  sv->redraw();
  */
}

void SliceAdjustmentTool::toolWheelEvent(QWheelEvent* ev) {
  if (!this->sv->dataSet()) {
    qDebug() << "Slice adjustmentTool wheekEvent if true;";
    return;
  }
  qreal direction = ev->delta() > 0 ? 1 : -1;
  moveSlice(this->sv->slice(),
            direction * (shiftDown ? this->fineAdjustmentFactor : 1));
}

void SliceAdjustmentTool::toolMousePressEvent(QMouseEvent* ev) {
  if (!this->sv->dataSet()) {
    return;
  }
  if (ev->button() == Qt::LeftButton) {
    if (this->ctrlDown) {
      // rotation Adjustment
      this->rotatingInProgress = true;
      QVector2D cursorOnPlane =
          QVector2D(this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true))
              .normalized();
      this->rotationHypotenuse = cursorOnPlane;
    } else {
      // tilt adjustment
      float tiltAngle = this->tiltAngle;  // in degrees
      if (shiftDown) tiltAngle *= fineAdjustmentFactor;

      QPointF cursorOnPlane =
          this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true);
      QVector3D cursorInVolume = this->sv->slice()
                                     ->getCuttingPlane()
                                     .get3DPoint(cursorOnPlane)
                                     .normalized();
      QVector3D rotationAxis =
          QVector3D::crossProduct(cursorInVolume, this->sv->slice()->normal());

      rotateSlice(this->sv->slice(), rotationAxis, tiltAngle);
    }
  } else if (ev->button() == Qt::RightButton) {
    // set origin to cursor
    QPointF cursorOnPlane =
        this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true);
    this->sv->slice()->setOrigin(cursorOnPlane);
  }
}

void SliceAdjustmentTool::toolMouseMoveEvent(QMouseEvent* ev) {
  if (!this->sv->dataSet()) {
    return;
  }
  if (this->dragUpdates) {
    if (this->ctrlDown) {
      if (this->rotatingInProgress) {
        QVector2D cursorOnPlane =
            QVector2D(this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true))
                .normalized();
        float angle = (float)qAcos(
            QVector3D::dotProduct(this->rotationHypotenuse, cursorOnPlane));
        // check clockwise or counterclockwise rotation
        float sign = 1;
        {
          /* to know if cursor is left of hypotenuse rotate their
           * coordinatesystem so that hypotenuse points in xAxis direction and
           * check if cursor.y is positive. Rotation matrix for this can be
           * obtained from hypotenuse since its normalized counterclockwise
           * clockwise cos(a)  -sin(a)                 cos(a)/det
           * sin(a)/det sin(a)   cos(a)                -sin(a)/det
           * cos(a)/det
           *
           * with cos(a) = hypotenuse.x , sin(a) = hypotenuse.y , det =
           * determinant(clockwise rotMat)
           */
          qreal cos = this->rotationHypotenuse.x();
          qreal sin = this->rotationHypotenuse.y();
          qreal det = cos * cos + sin * sin;
          // y-part of matrix multiplication (clockwise*cursor)
          qreal rotCursorY =
              -sin / det * cursorOnPlane.x() + cos / det * cursorOnPlane.y();
          sign = rotCursorY > 0 ? 1 : -1;
        }
        QVector3D rotationAxis = this->sv->slice()->normal();
        if (xDown) rotationAxis = this->sv->slice()->xAxis();
        if (yDown) rotationAxis = this->sv->slice()->yAxis();
        rotateSlice(this->sv->slice(), rotationAxis,
                    ((angle) / 3.1415f) * 180 * sign);
        this->rotationHypotenuse = cursorOnPlane;
      }
    }
  }
}

void SliceAdjustmentTool::toolMouseReleaseEvent(QMouseEvent* ev) {
  if (this->ctrlDown) {
    if (this->rotatingInProgress) {
      this->rotatingInProgress = false;
      QVector2D cursorOnPlane =
          QVector2D(this->sv->sliceImage().pixelToPlanePoint(ev->pos(), true))
              .normalized();
      float angle = (float)qAcos(
          QVector3D::dotProduct(this->rotationHypotenuse, cursorOnPlane));
      // check clockwise or counterclockwise rotation
      float sign = 1;
      {
        /* to know if cursor is left of hypotenuse rotate their coordinatesystem
         * so that hypotenuse is points in xAxis direction and check if cursor.y
         * is positive. Rotation matrix for this can be obtained from hypotenuse
         * since its normalized counterclockwise                clockwise cos(a)
         * -sin(a)                 cos(a)/det   sin(a)/det sin(a)   cos(a)
         * -sin(a)/det   cos(a)/det
         *
         * with cos(a) = hypotenuse.x , sin(a) = hypotenuse.y , det =
         * determinant(clockwise rotMat)
         */
        qreal cos = this->rotationHypotenuse.x();
        qreal sin = this->rotationHypotenuse.y();
        qreal det = cos * cos + sin * sin;
        // y-part of matrix multiplication (clockwise*cursor)
        qreal rotCursorY =
            -sin / det * cursorOnPlane.x() + cos / det * cursorOnPlane.y();
        sign = rotCursorY > 0 ? 1 : -1;
      }
      QVector3D rotationAxis = this->sv->slice()->normal();
      if (xDown) rotationAxis = this->sv->slice()->xAxis();
      if (yDown) rotationAxis = this->sv->slice()->yAxis();
      rotateSlice(this->sv->slice(), rotationAxis,
                  ((angle) / 3.1415f) * 180 * sign);
    }
  }
}

void SliceAdjustmentTool::toolKeyPressEvent(QKeyEvent* ev) {
  switch (ev->key()) {
    case Qt::Key_Shift:
      this->shiftDown = true;
      break;
    case Qt::Key_Control:
      this->ctrlDown = true;
      break;
    case Qt::Key_X:
      this->yDown = false;
      this->xDown = true;
      break;
    case Qt::Key_Y:
      this->xDown = false;
      this->yDown = true;
      break;
    // arrow keys
    case Qt::Key_Up:
      rotateSlice(
          this->sv->slice(), this->sv->slice()->xAxis(),
          this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1));
      break;
    case Qt::Key_Down:
      rotateSlice(
          this->sv->slice(), -this->sv->slice()->xAxis(),
          this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1));
      break;
    case Qt::Key_Right:
      rotateSlice(
          this->sv->slice(), this->sv->slice()->yAxis(),
          this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1));
      break;
    case Qt::Key_Left:
      rotateSlice(
          this->sv->slice(), -this->sv->slice()->yAxis(),
          this->tiltAngle * (this->shiftDown ? this->fineAdjustmentFactor : 1));
      break;
    case Qt::Key_PageUp:
      moveSlice(this->sv->slice(),
                (shiftDown ? this->fineAdjustmentFactor : 1));
      break;
    case Qt::Key_PageDown:
      moveSlice(this->sv->slice(),
                (shiftDown ? -this->fineAdjustmentFactor : -1));
      break;
    default:
      break;
  }
}

void SliceAdjustmentTool::toolKeyReleaseEvent(QKeyEvent* ev) {
  switch (ev->key()) {
    case Qt::Key_Shift:
      this->shiftDown = false;
      break;
    case Qt::Key_Control:
      this->ctrlDown = false;
      this->rotatingInProgress = false;
      break;
    case Qt::Key_X:
      this->xDown = false;
      break;
    case Qt::Key_Y:
      this->yDown = false;
      break;
    default:
      break;
  }
}

void SliceAdjustmentTool::rotateSlice(vx::Slice* slice,
                                      const QVector3D& rotationAxis,
                                      float rotationAngle) {
  if (std::isnan(rotationAngle)) {
    return;
  }
  QQuaternion currentRotation = slice->rotation();
  QQuaternion rotationAdjustment =
      QQuaternion::fromAxisAndAngle(rotationAxis, rotationAngle);
  auto rotation = rotationAdjustment * currentRotation;
  slice->setRotation(rotation);
}

void SliceAdjustmentTool::moveSlice(vx::Slice* slice, float steps) {
  auto data = slice->getDataset() ? slice->getDataset()->volumeData()
                                  : QSharedPointer<VolumeData>();
  if (!data) {
    qWarning() << "SliceAdjustmentTool::moveSlice: data is null";
    return;
  }

  auto normal = slice->normal();
  auto stepSize =
      data->getStepSize(vx::vectorCast<double>(vx::toVector(normal)));
  // if (auto vol = qSharedPointerDynamicCast<VolumeDataVoxel>(data))
  //   qDebug() << vol->getSpacing();
  // qDebug() << normal << stepSize;
  slice->translateAlongNormal(steps * stepSize);
}

SliceAdjustmentLayer::SliceAdjustmentLayer(SliceVisualizer* sv) : sv(sv) {
  // Redraw when the slice center is shown / hidden
  QObject::connect(sv->properties, &SliceProperties::showSliceCenterChanged,
                   this, &Layer::triggerRedraw);

  // Ignore changes to the slice, they don't affect the lines drawn here
  /*
  QObject::connect(sv->properties,
                   &SliceProperties::orientationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::originChanged,
                   this, &Layer::triggerRedraw);
  */

  // Redraw when shown part of slice changes
  connect(sv->properties, &SliceProperties::centerPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv->properties, &SliceProperties::verticalSizeChanged, this,
          &Layer::triggerRedraw);
}

void SliceAdjustmentLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.showSliceCenter()) return;

  QPainter painter(&outputImage);

  QRectF area =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());
  qreal origX = -area.left();
  // qreal origY = -area.top();
  qreal origY = area.bottom();
  // normalize x & y
  origX /= area.width();
  origY /= area.height();
  // stretch x & y to canvas
  origX *= outputImage.width();
  origY *= outputImage.height();

  // draw x axis
  QPen pen(QColor(0x00, 0xff, 0x00));
  painter.setPen(pen);
  painter.drawLine(0, (int)origY, outputImage.width(), (int)origY);
  // draw y axis
  pen.setColor(QColor(0x00, 0x00, 0xff));
  painter.setPen(pen);
  painter.drawLine((int)origX, 0, (int)origX, outputImage.height());
}
