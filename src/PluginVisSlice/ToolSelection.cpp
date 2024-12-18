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

#include "ToolSelection.hpp"

#include <Voxie/Node/ParameterCopy.hpp>

#include <PluginVisSlice/Prototypes.hpp>
#include <PluginVisSlice/SliceVisualizer.hpp>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>

using namespace vx;
using namespace vx::filter;

ToolSelection::ToolSelection(QWidget* parent, SliceVisualizer* sv)
    : Visualizer2DTool(parent), sv(sv) {
  layer_ = ToolSelectionLayer::create(sv);

  QGridLayout* layout = new QGridLayout(this);
  layout->setSpacing(2);
  rectangleButton =
      new QPushButton(QIcon(":/icons/layer-shape.png"), "Rectangle");
  rectangleButton->setCheckable(true);
  connect(rectangleButton, &QPushButton::clicked, [=]() {
    rectangleActive = true;
    ellipseActive = false;
    this->deleteButton->setChecked(false);
    polygonActive = false;
    this->sv->switchToolTo(this);
  });
  layout->addWidget(rectangleButton, 0, 0);
  rectangleButton->hide();  // show();

  ellipseButton =
      new QPushButton(QIcon(":/icons/layer-shape-ellipse.png"), "Ellipse");
  ellipseButton->setCheckable(true);
  connect(ellipseButton, &QPushButton::clicked, [=]() {
    ellipseActive = true;
    rectangleActive = false;
    polygonActive = false;
    //  this->deleteActive = false;
    this->deleteButton->setChecked(false);
    this->sv->switchToolTo(this);
  });
  layout->addWidget(ellipseButton, 0, 1);
  ellipseButton->hide();  // show();

  polygonButton =
      new QPushButton(QIcon(":/icons/layer-shape-polygon.png"), "Polygon");
  polygonButton->setCheckable(true);
  connect(polygonButton, &QPushButton::clicked, [=]() {
    this->polygonActive = true;
    this->ellipseActive = false;
    this->rectangleActive = false;
    this->deleteButton->setChecked(false);
    this->deleteActive = false;
    this->sv->switchToolTo(this);
  });
  layout->addWidget(polygonButton, 1, 0);
  polygonButton->hide();  // show();

  deleteButton = new QPushButton(QIcon(":/icons/layer-shade.png"), "Clear");
  deleteButton->setCheckable(true);
  connect(deleteButton, &QPushButton::clicked, [=]() {
    this->polygonActive = false;
    this->ellipseActive = false;
    this->rectangleActive = false;
    // this->deleteActive = true;
    if (mask != nullptr) {
      mask->clearMask();
      this->polygonButton->setChecked(false);
      this->rectangleButton->setChecked(false);
      this->ellipseButton->setChecked(false);
      // TODO this->draw();
    }
    this->sv->switchToolTo(this);
  });
  layout->addWidget(deleteButton, 1, 1);
  deleteButton->hide();  // show();

  this->setLayout(layout);
}

QString ToolSelection::getName() { return "Selection 2D"; }

QIcon ToolSelection::getIcon() { return QIcon(":/icons/ruler--pencil.png"); }

void ToolSelection::activateTool() {
  // qDebug() << "activateTool";
  if (mask) {
    // toolActive = true;
    rectangleButton->show();
    ellipseButton->show();
    polygonButton->show();
    deleteButton->show();
    //
    if (rectangleActive) {
      rectangleButton->setChecked(true);
    } else if (ellipseActive) {
      ellipseButton->setChecked(true);
    } else if (polygonActive) {
      polygonButton->setChecked(true);
    }
  } else {
    QMessageBox messageBox(vx::voxieRoot().mainWindow());
    messageBox.critical(
        0, "Error",
        "Can't activate mask tool without enabling it via FilterChain first.");
    messageBox.setFixedSize(500, 200);
  }
}

void ToolSelection::deactivateTool() {
  // qDebug() << "deactivateTool";
  // toolActive = false;
  setMask(nullptr);
  rectangleButton->setChecked(false);
  ellipseButton->setChecked(false);
  polygonButton->setChecked(false);
  deleteButton->setChecked(false);
  firstValue = true;
  //
  rectangleButton->hide();
  ellipseButton->hide();
  polygonButton->hide();
  deleteButton->hide();

  sv->properties->setShow2DFilterMask(0);
}

void ToolSelection::setMask(vx::filter::Filter2D* filter) {
  // qDebug() << "setMask" << filter;
  this->filter = filter;
  if (filter) {
    this->mask = filter->getMask();
    this->sv->properties->setShow2DFilterMask(filter->filterID);
  } else {
    this->mask = nullptr;
    this->sv->properties->setShow2DFilterMask(0);
  }
}

void ToolSelection::toolMousePressEvent(QMouseEvent* e,
                                        const vx::Vector<double, 2>& pixelPos) {
  if (mask != nullptr) {
    auto planePos = this->sv->pixelPosToPlanePosCurrentImage(pixelPos);
    // TODO: Probably plane coordinates should be used here
    start = e->pos();
    if (rectangleActive) {
      // qDebug() << "bla";
      // if (firstValue)
      //{
      // firstValue = false;
      layer()->setPreview(QPainterPath());
      startRect = planePos;
      mousePressed = true;
      /*} else {
          qDebug() << startRect;
          qDebug()<< tempPos;
          QPointF endRect;
          mask->addRectangle(startRect.x(), startRect.y(), endRect.x(),
      endRect.y()); qDebug() << endRect; firstValue = true; this->draw();
      }*/
    }

    if (polygonActive) {
      this->previewPolygon.append(e->pos());
      this->polygon.append(QPointF(planePos[0], planePos[1]));
      QPainterPath temp;
      temp.addPolygon(QPolygonF(this->previewPolygon));
      layer()->setPreview(temp);
    }

    if (ellipseActive) {
      // if (firstValue)
      //{
      middlePointEllipse = planePos;
      //  firstValue = false;
      layer()->setPreview(QPainterPath());
      mousePressed = true;
      /*} else {
          QPointF radius;
          this->mask->addEllipse(this->middlePointEllipse.x(),
      this->middlePointEllipse.y(), fabs(this->middlePointEllipse.x() -
      radius.x()), fabs(this->middlePointEllipse.x() - radius.y())); firstValue
      = true; this->draw();
      }*/
    }
  }
}

void ToolSelection::toolMouseMoveEvent(QMouseEvent* e,
                                       const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(pixelPos);

  if (mousePressed) {
    if (rectangleActive) {
      QPoint tempEnd = e->pos();
      QPoint tempStart = this->start;

      if (start.x() < tempEnd.x() && start.y() > tempEnd.y()) {
        // startX = endX - fabs(endX - startX);
        tempStart.setY(tempEnd.y());
      }
      // falls startpunkt oben rechts und endpunkt unten links
      else if (start.x() > tempEnd.x() && start.y() < tempEnd.y()) {
        tempStart.setX(tempEnd.x());
        // startY = startY - fabs(startY - endY);
      }
      // falls startpunkt unten rechts und endpunkt oben links
      else if (start.x() > tempEnd.x() && start.y() > tempEnd.y()) {
        // int temp = tempStart.x();
        tempStart.setX(tempEnd.x());
        // tempEnd.setX(temp);
        // temp  = tempStart.y();
        tempStart.setY(tempEnd.y());
        // tempEnd.setY(temp);
      }
      QPainterPath temp;
      temp.addRect(QRectF((qreal)tempStart.x(), (qreal)tempStart.y(),
                          fabs((qreal)tempEnd.x() - (qreal)start.x()),
                          fabs((qreal)tempEnd.y() - (qreal)start.y())));
      layer()->setPreview(temp);
    }
    if (ellipseActive) {
      QPainterPath temp;
      temp.addEllipse(QPointF((qreal)start.x(), (qreal)start.y()),
                      fabs((qreal)e->pos().x() - (qreal)start.x()),
                      fabs((qreal)e->pos().y() - (qreal)start.y()));
      layer()->setPreview(temp);
    }
  }
}

void ToolSelection::toolMouseReleaseEvent(
    QMouseEvent* e, const vx::Vector<double, 2>& pixelPos) {
  Q_UNUSED(e);

  if (polygonActive) {
    return;
  }
  mousePressed = false;
  layer()->clearPreview();

  auto planePos = this->sv->pixelPosToPlanePosCurrentImage(pixelPos);
  if (rectangleActive) {
    mask->addRectangle(startRect[0], startRect[1], planePos[0], planePos[1]);
  }

  if (ellipseActive) {
    mask->addEllipse(this->middlePointEllipse[0], this->middlePointEllipse[1],
                     fabs(planePos[0] - this->middlePointEllipse[0]),
                     fabs(planePos[1] - this->middlePointEllipse[1]));
  }
}

void ToolSelection::toolKeyPressEvent(QKeyEvent* e) {
  if (polygonActive && mask != nullptr && polygon.size() > 2) {
    if (e->key() == Qt::Key_Space) {
      polygon.append(polygon.at(0));
      mask->addPolygon(this->polygon);
      polygon.clear();
      this->previewPolygon.clear();
      this->layer()->clearPreview();
    }
  }
}

/*
 * calculates the plante Coordinates to Image coordinates.
 *
 * Used for painting.
 */
QPainterPath ToolSelectionLayer::planeToImage(const QRectF& planeArea,
                                              const QSize& canvasSize,
                                              const QPainterPath& path) {
  QPainterPath returnPath;
  // qreal x = ((path.boundingRect().left() -
  // planeArea.left())/(planeArea.width())) *
  // canvasSize.width();  qreal y = ((path.boundingRect().top() -
  // planeArea.top())/(planeArea.height())) *
  // canvasSize.height();

  // qDebug() << path.boundingRect().left() << path.boundingRect().top();
  // qDebug() << x << y;
  // QTransform translate(1, 0, 0, 1, -planeArea.left(),
  // -planeArea.top());
  QTransform translate(1, 0, 0, 1, -planeArea.left(), -planeArea.bottom());
  qreal scaleX = canvasSize.width() / planeArea.width();
  qreal scaleY = canvasSize.height() / planeArea.height();

  // QTransform scale(scaleX, 0, 0, scaleY, 0, 0);
  QTransform scale(scaleX, 0, 0, -scaleY, 0, 0);

  returnPath = translate.map(path);
  returnPath = scale.map(returnPath);

  return returnPath;
}

ToolSelectionLayer::ToolSelectionLayer(SliceVisualizer* sv) {
  // Redraw when any of the filter properties changes
  QObject::connect(sv->properties,
                   &SliceProperties::filter2DConfigurationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::show2DFilterMaskChanged,
                   this, &Layer::triggerRedraw);

  // Redraw when the slice changes
  // This is needed because slice changes might also trigger changes of the
  // filter position, but this does not emit a filter2DConfigurationChanged
  // signal currently.
  // TODO: Properly emit filter2DConfigurationChanged whenever the filter
  // changes
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

void ToolSelectionLayer::clearPreview() {
  QMutexLocker locker(&this->previewMutex_);
  this->previewSet_ = false;
  this->preview_ = QPainterPath();
  this->triggerRedraw();
}
void ToolSelectionLayer::setPreview(const QPainterPath& preview) {
  QMutexLocker locker(&this->previewMutex_);
  this->previewSet_ = true;
  this->preview_ = preview;
  this->triggerRedraw();
}

void ToolSelectionLayer::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters,
    bool isMainImage) {
  Q_UNUSED(isMainImage);

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  auto planeArea =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  auto filterId = properties.show2DFilterMask();
  if (filterId == 0) return;

  // TODO: Check whether this leaks filter objects
  FilterChain2D filterChain;
  filterChain.fromXMLString(properties.filter2DConfiguration());
  Filter2D* filter = filterChain.getFilterByIDOrNull(filterId);
  if (!filter) {
    qWarning() << "ToolSelectionLayer: Could not find filter";
    return;
  }
  auto mask = filter->getMask();

  QPainter painter(&outputImage);

  painter.setPen(QColor(255, 0, 0));
  QBrush brush;
  brush.setColor(QColor(122, 163, 39));

  // painter.fillPath(this->planeToImage(planeArea, outputImage.size(),
  // mask->getPath()), QColor(122, 163, 39));
  painter.drawPath(
      this->planeToImage(planeArea, outputImage.size(), mask->getPath()));

  QPainterPath previewPath;
  {
    QMutexLocker locker(&this->previewMutex_);
    if (!this->previewSet_) return;
    previewPath = this->preview_;
  }

  painter.setPen(QColor(255, 255, 0));
  painter.drawPath(previewPath);
}
