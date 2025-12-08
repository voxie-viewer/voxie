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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "LinePlotVisualizer.hpp"

#include <PluginVisLinePlot/Prototypes.hpp>

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/VolumeNode.hpp>

#include <Voxie/Gui/ErrorMessage.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyUI.hpp>

#include <Voxie/IO/SaveFileDialog.hpp>

#include <Voxie/Vis/LabeledAxis.hpp>

#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/VolumeData.hpp>

#include <VoxieClient/Array.hpp>
#include <VoxieClient/Format.hpp>

#include <QtGui/QColor>
#include <QtGui/QPainter>

#include <QtCore/QDebug>

using namespace vx;
using namespace vx::visualization;

VX_NODE_INSTANTIATION(LinePlotVisualizer)

LinePlotVisualizer::LinePlotVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()),
      properties(new PropertiesType(this)) {
  view()->setMinimumSize(300 / 96.0 * this->view()->logicalDpiX(),
                         200 / 96.0 * this->view()->logicalDpiY());

  QWidget* section = new QWidget;
  section->setLayout(new QVBoxLayout());
  section->layout()->setContentsMargins(0, 0, 0, 0);
  section->setWindowTitle(tr("Mapping"));
  addPropertySection(section);

  auto createPlaneNodeAction = new QAction("Export data", this);
  QObject::connect(createPlaneNodeAction, &QAction::triggered, this,
                   &LinePlotVisualizer::exportData);
  addContextMenuAction(createPlaneNodeAction);

  // Trigger redraw on change of volume data
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &LinePlotProperties::volume,
      &LinePlotProperties::volumeChanged, &DataNode::dataChangedFinished, this,
      &LinePlotVisualizer::triggerRedraw);

  // Trigger redraw on change of volume position / orientation
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &LinePlotProperties::volume,
      &LinePlotProperties::volumeChanged, &VolumeNode::translationChanged, this,
      &LinePlotVisualizer::triggerRedraw);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &LinePlotProperties::volume,
      &LinePlotProperties::volumeChanged, &VolumeNode::rotationChanged, this,
      &LinePlotVisualizer::triggerRedraw);

  // Trigger redraw on change of geometric primitive data
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &LinePlotProperties::geometricPrimitive,
      &LinePlotProperties::geometricPrimitiveChanged,
      &DataNode::dataChangedFinished, this, &LinePlotVisualizer::triggerRedraw);

  // Trigger redraw when the select start/end point changes
  QObject::connect(properties, &LinePlotProperties::startPointChanged, this,
                   &LinePlotVisualizer::triggerRedraw);
  QObject::connect(properties, &LinePlotProperties::endPointChanged, this,
                   &LinePlotVisualizer::triggerRedraw);
}

LinePlotVisualizer::~LinePlotVisualizer() {}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
LinePlotVisualizer::getRenderFunction() {
  return [](const QSharedPointer<ImageDataPixel>& outputImage,
            const VectorSizeT2& outputRegionStart, const VectorSizeT2& size,
            const QSharedPointer<ParameterCopy>& parameters,
            const QSharedPointer<VisualizerRenderOptions>& options) {
    Q_UNUSED(options);

    auto pointsOpt = getData(parameters);
    if (!pointsOpt) return;
    const std::vector<vx::Vector<float, 2>>& points = *pointsOpt;

    auto x_min = std::numeric_limits<float>::max();
    auto x_max = std::numeric_limits<float>::min();
    auto y_min = std::numeric_limits<float>::max();
    auto y_max = std::numeric_limits<float>::min();
    for (const auto& point : points) {
      x_min = std::min(x_min, point[0]);
      x_max = std::max(x_max, point[0]);
      y_min = std::min(y_min, point[1]);
      y_max = std::max(y_max, point[1]);
    }

    QImage image(static_cast<int>(size.x), static_cast<int>(size.y),
                 QImage::Format_ARGB32);
    // image.fill(qRgba(0, 0, 0, 0));

    // TODO
    auto dpiScale = 1.0;
    auto backgroundColor = QColor(20, 20, 20, 255);
    auto foregroundColor = QColor(200, 200, 200, 255);

    // TODO: Distinguish between axisMarginsB and axisMarginsL (same for
    // axisMarginsTR) to allow different dpiScaleX and dpiScaleY?
    static const float axisMarginsBL = 40.f * dpiScale;
    static const float axisMarginsTR = 5.f * dpiScale;

    QRectF rect(axisMarginsBL, axisMarginsTR,
                image.width() - axisMarginsTR - axisMarginsBL,
                image.height() - axisMarginsTR - axisMarginsBL);

    QPainter painter(&image);

    painter.fillRect(0, 0, image.width(), image.height(), backgroundColor);

    painter.setClipRect(rect);

    auto x_to_display = [&](float v) {
      // if (xAxisLog) v = std::log(v);
      v = rect.left() + (v - x_min) / (x_max - x_min) * rect.width();
      return v;
    };

    auto y_to_display = [&](float v) {
      // if (yAxisLog) v = std::log(v);
      v = rect.top() + (y_max - v) / (y_max - y_min) * rect.height();
      return v;
    };

    QVector<QLineF> lines;
    for (size_t i = 0; i + 1 < points.size(); i++) {
      auto point1 = points[i];
      auto point2 = points[i + 1];

      lines << QLineF(QPointF(x_to_display(point1.access<0>()),
                              y_to_display(point1.access<1>())),
                      QPointF(x_to_display(point2.access<0>()),
                              y_to_display(point2.access<1>())));
    }
    // qDebug() << lines;
    // qDebug() << rect;
    painter.setPen(foregroundColor);
    painter.setBrush(foregroundColor);
    painter.drawLines(lines);

    QRectF axisRect(rect.left(), rect.bottom(), rect.width(), -rect.height());

    painter.setClipping(false);

    LabeledAxis axisX(LabeledAxis::Horizontal);
    axisX.setColor(foregroundColor);
    axisX.setLowerValue(x_min);
    axisX.setUpperValue(x_max);
    axisX.setRect(axisRect);
    axisX.setLogScale(false);
    axisX.setLabel("Position [m]");
    axisX.draw(dpiScale, painter);

    LabeledAxis axisY(LabeledAxis::Vertical);
    axisY.setColor(foregroundColor);
    axisY.setLowerValue(y_min);
    axisY.setUpperValue(y_max);
    axisY.setRect(axisRect);
    axisY.setLogScale(false);
    axisY.setLabel("Value");
    axisY.draw(dpiScale, painter);

    // drawing lines with x- and y-values on mouse hover
    // TODO
#if 0
    QPoint cursorPos = mapFromGlobal(QCursor::pos());
    if (underMouse() && cursorPos.x() > axisMarginsBL &&
        cursorPos.x() < width() - axisMarginsTR &&
        cursorPos.y() > axisMarginsTR &&
        cursorPos.y() < height() - axisMarginsBL) {
      painter.setPen(QPen(foregroundColor));

      // draw vertical line at cursor x
      painter.drawLine(cursorPos.x(), axisMarginsTR, cursorPos.x(),
                       height() - axisMarginsBL);

      // calculate the x-value at the cursor pos
      float xValue =
          unscaleX((cursorPos.x() - axisMarginsBL) /
                       (width() - axisMarginsBL - axisMarginsTR) * (max - min) +
                   min);

      float yValue = 0;
      if (isHoverValueSnappingEnabled()) {
        // get the index of the bucket at the cursor pos
        qint64 index =
            histogramProvider()->getData()->bucketIndexOfValue(xValue);
        // then use the value of the bucket at the cursor pos as y value
        yValue = histogramProvider()->getData()->buckets[index];

        // scale it if we have a log scale, then draw the horizontal line
        auto yValueScaled =
            (yAxisLog ? logf(yValue + 1) : yValue) * invMaxCount;
        painter.drawLine(
            axisMarginsBL,
            rect.top() + std::floor((1.0f - yValueScaled) * rect.height()),
            width() - axisMarginsTR,
            rect.top() + std::floor((1.0f - yValueScaled) * rect.height()));
      } else {
        // calculate the y-value at the cursor pos. The first line calculates a
        // fraction (0.0 to 1.0) depending on where the cursor is. The second
        // line uses the fraction and upper and lower bounds of the coordinate
        // system to calculate the value at the cursor pos
        yValue = unscaleY(
            (1.0f - (cursorPos.y() - axisMarginsTR) /
                        (height() - axisMarginsBL - axisMarginsTR)) *
            (scaleY(y_max) - scaleY((yAxisLog ? 1 : 0))));

        // draw horizontal line at cursor y
        painter.drawLine(axisMarginsBL, cursorPos.y(), width() - axisMarginsTR,
                         cursorPos.y());
      }

      // print the value at the cursor position under the histogram
      QString xValueString = QString::number(xValue, 'f', 2);
      float textWidth =
          QFontMetrics(painter.font()).boundingRect(xValueString).width();
      // draw the text at the correct position
      painter.drawText(QPoint(cursorPos.x() - textWidth / 2, height() - 10),
                       xValueString);

      // display the y value. Number of decimal places gets chosen to try to
      // keep the number to a max length of 5 digits´
      int digitCount = log10f(yValue) + 1;
      QString yValueString =
          QString::number(yValue, 'f', qMax(5 - digitCount, 0));
      float textHeight = QFontMetrics(painter.font()).height();
      painter.drawText(QPoint(1, cursorPos.y() + textHeight / 2), yValueString);
    }
#endif

    outputImage->fromQImage(image, outputRegionStart);
  };
}

std::shared_ptr<std::vector<vx::Vector<float, 2>>> LinePlotVisualizer::getData(
    const QSharedPointer<ParameterCopy>& parameters) {
  LinePlotPropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  // TODO
  InterpolationMethod interpolationMethod = InterpolationMethod::Linear;

  // qDebug() << "LinePlotVisualizer::renderFunction" << size.x << size.y;

  QDBusObjectPath geomRaw = properties.geometricPrimitiveRaw();
  if (geomRaw == QDBusObjectPath("/")) {
    qDebug() << "No geom connected, nothing to render";
    return nullptr;
  }
  auto geomData = qSharedPointerDynamicCast<GeometricPrimitiveData>(
      parameters->getData(geomRaw).data());
  if (!geomData) {
    qDebug() << "No geom data, nothing to render";
    return nullptr;
  }

  auto startPointID = properties.startPoint();
  auto endPointID = properties.endPoint();
  auto startPoint = startPointID == 0
                        ? QSharedPointer<GeometricPrimitivePoint>()
                        : qSharedPointerDynamicCast<GeometricPrimitivePoint>(
                              geomData->getPrimitiveOrNull(startPointID));
  auto endPoint = endPointID == 0
                      ? QSharedPointer<GeometricPrimitivePoint>()
                      : qSharedPointerDynamicCast<GeometricPrimitivePoint>(
                            geomData->getPrimitiveOrNull(endPointID));
  if (!startPoint || !endPoint) {
    qDebug() << "Unable to find start or end point or is not a point";
    return nullptr;
  }

  VolumePropertiesCopy volumeProperties(
      parameters->properties()[properties.volumeRaw()]);

  QDBusObjectPath volumeRaw = properties.volumeRaw();
  if (volumeRaw == QDBusObjectPath("/")) {
    qDebug() << "No volume connected, nothing to render";
    return nullptr;
  }

  auto data = qSharedPointerDynamicCast<VolumeData>(
      parameters->getData(volumeRaw).data());
  QQuaternion adjustedRotation = volumeProperties.rotation();
  QVector3D adjustedPosition = volumeProperties.translation();
  if (!data) {
    qDebug() << "No volume data, nothing to render";
    return nullptr;
  }

  // TODO: Move this somewhere else
  vx::AffineMap<double, 3> objectToGlobal =
      vx::createTranslation(vectorCast<double>(toVector(adjustedPosition))) *
      vx::Rotation<double, 3>(
          quaternionCast<double>(toQuaternion(adjustedRotation)));
  vx::AffineMap<double, 3> globalToObject = vx::inverse(objectToGlobal);

  auto startPointObject = globalToObject.map(startPoint->positionVec());
  auto endPointObject = globalToObject.map(endPoint->positionVec());
  using std::sqrt;
  auto length = sqrt(vx::squaredNorm(endPointObject - startPointObject));

  // TODO: What should be used here?
  auto stepSizePlanned =
      (data->getStepSize({0, 0, 1}) + data->getStepSize({0, 1, 0}) +
       data->getStepSize({1, 0, 0})) /
      3;
  stepSizePlanned /= 10;

  auto stepCountDouble = std::ceil(length / stepSizePlanned);
  // qDebug() << length << stepSizePlanned << stepCountDouble;
  if (stepCountDouble > 1000000) {
    qDebug() << "LinePlotVisualizer: Too many steps (" << stepCountDouble
             << "), not plotting";
    return nullptr;
  }
  auto stepSize = length / stepCountDouble;
  size_t stepCount = (size_t)stepCountDouble;
  size_t pointCount = stepCount + 1;

  vx::Array1<float> lineValues({pointCount});
  data->extractLine(startPointObject, endPointObject, interpolationMethod,
                    lineValues, 0, pointCount);

  auto points = std::make_shared<std::vector<vx::Vector<float, 2>>>(pointCount);
  for (size_t i = 0; i < points->size(); i++) {
    vx::Vector<float, 2> point{
        static_cast<float>(stepSize * static_cast<double>(i)), lineValues(i)};
    (*points)[i] = point;
  }

  return points;
}

void LinePlotVisualizer::exportData() {
  auto parameters = ParameterCopy::getParameters(this);

  auto pointsOpt = getData(parameters);
  if (!pointsOpt) {
    // TODO: Better error message?
    vx::showErrorMessage("Line data exporter", "Unable to get line data");
    return;
  }

  auto dialog = makeSharedQObject<vx::io::SaveFileDialog>(
      voxieRoot().mainWindow(), QObject::tr("Select point output file"),
      QString());
  dialog->addFilter(vx::io::FilenameFilter("CSV file", {"*.csv"}), nullptr);
  QWeakPointer<vx::io::SaveFileDialog> dialogWeak = dialog;

  dialog->setup();
  // strongRef keeps a reference while the dialog is being shown
  auto strongRef =
      createQSharedPointer<QSharedPointer<vx::io::SaveFileDialog>>();
  QObject::connect(dialog.data(), &QDialog::finished,
                   [dialogWeak, strongRef, pointsOpt](int result) {
                     // qDebug() << "finished" << result;
                     auto dialog2 = dialogWeak.lock();
                     strongRef->reset();
                     if (!result) return;

                     try {
                       if (!dialog2)
                         throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                             "Dialog is deleted");

                       auto fn = dialog2->selectedFiles().first();
                       if (fn == "") {
                         vx::showErrorMessage("Error while saving CSV file",
                                              "No file selected");
                         return;
                       }
                       // qDebug() << fn;

                       QFile file(fn);
                       if (!file.open(QIODevice::WriteOnly)) {
                         vx::showErrorMessage(
                             "Error while saving CSV file",
                             "Unable to open file" + file.errorString());
                         return;
                       }

                       QTextStream output(&file);
                       for (const auto& point : *pointsOpt) {
                         for (size_t col = 0; col < 2; col++) {
                           if (col > 0) output << ",";
                           output << vx::format("{}", point[col]);
                         }
                         output << "\r\n";
                       }
                     } catch (vx::Exception& e) {
                       vx::showErrorMessage("Error while saving CSV file", e);
                     }
                   });
  *strongRef = dialog;
  dialog->show();
}
