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

#include "InfoWidget.hpp"

#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>

#include <PluginVisRaw/Prototypes.hpp>
#include <PluginVisRaw/RawData2DVisualizer.hpp>

#include <QtWidgets/QLabel>

InfoWidget::InfoWidget(RawVisualizer* rv, QWidget* parent)
    : QWidget(parent), rv(rv) {
  this->layout = new QVBoxLayout();
  this->layout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(layout);

  currentStream = new QLabel();
  this->layout->addWidget(currentStream);

  currentImageId = new QLabel();
  this->layout->addWidget(currentImageId);

  versionMetadataLabel = new QLabel();
  this->layout->addWidget(versionMetadataLabel);

  geometryInfo = new QLabel();
  this->layout->addWidget(geometryInfo);
  geometryInfo->setMinimumHeight(50);

  auto labelPosMouse = new QLabel("");
  this->layout->addWidget(labelPosMouse);

  auto labelPos2D = new QLabel("");
  this->layout->addWidget(labelPos2D);

  auto labelPosPixel = new QLabel("");
  this->layout->addWidget(labelPosPixel);

  auto labelPosPixelRounded = new QLabel("");
  this->layout->addWidget(labelPosPixelRounded);

  auto labelValue = new QLabel("");
  this->layout->addWidget(labelValue);

  QObject::connect(
      rv->view(), &vx::visualization::VisualizerView::forwardMouseMoveEvent,
      this,
      [this, labelPosMouse, labelPos2D, labelPosPixel, labelPosPixelRounded,
       labelValue](QMouseEvent* e) {
        // qDebug() << e;

        auto width = this->rv->view()->width();
        auto height = this->rv->view()->height();
        auto area = this->rv->getCurrentPlaneArea(this->rv->properties,
                                                  QSize(width, height));

        double x = ((double)e->pos().x() + 0.5) / width;
        double y = (height - 1 - (double)e->pos().y() + 0.5) / height;
        double posX = area.x() + x * area.width();
        double posY = area.y() + y * area.height();
        // qDebug() << e->pos() << area << x << y << posX << posY;

        labelPosMouse->setText(QString("Mouse position: %1 %2")
                                   .arg(e->pos().x())
                                   .arg(e->pos().y()));
        labelPos2D->setText(
            QString("Position on detector: %1 %2").arg(posX).arg(posY));

        QSharedPointer<vx::TomographyRawData2DAccessor> data;
        auto dataNode = qobject_cast<vx::TomographyRawDataNode*>(
            this->rv->properties->rawData());
        if (dataNode)
          data = qSharedPointerDynamicCast<vx::TomographyRawData2DAccessor>(
              dataNode->data());
        if (data) {
          QSharedPointer<vx::DataVersion> dataVersion;
          QString stream;
          qint64 imageId;
          QJsonObject metadata;
          QJsonObject imageKind;
          QSharedPointer<vx::ImageDataPixel> rawImage;
          std::tie(dataVersion, stream, imageId, metadata, imageKind,
                   rawImage) = this->rv->cache->getMainImage();

          if (!rawImage) {
            labelPosPixel->setText("");
            labelPosPixelRounded->setText("");
            labelValue->setText("");
          } else {
            auto projArea = this->rv->getProjectionArea(
                data, stream, imageId,
                metadata["ProjectionGeometry"].toObject());

            // TODO: Is this correct?
            double posXPix =
                (posX - projArea.x()) / projArea.width() * rawImage->width();
            double posYPix =
                (posY - projArea.y()) / projArea.height() * rawImage->height();
            // TODO: Is this correct? Why is this needed? (Should not be needed
            // I think)
            posXPix += 0.5;
            posYPix += 0.5;
            // qDebug() << projArea << posXPix << posYPix;
            int64_t posXPixRounded = std::floor(posXPix);
            int64_t posYPixRounded = std::floor(posYPix);

            labelPosPixel->setText(QString("Position on detector pixel: %1 %2")
                                       .arg(posXPix)
                                       .arg(posYPix));
            labelPosPixelRounded->setText(
                QString("Position on detector pixel (rounded): %1 %2")
                    .arg(posXPixRounded)
                    .arg(posYPixRounded));

            if (posXPixRounded >= 0 &&
                (quint64)posXPixRounded < rawImage->width() &&
                posYPixRounded >= 0 &&
                (quint64)posYPixRounded < rawImage->height()) {
              // TODO: Different component count?
              auto value = rawImage->performInGenericContextWithComponents<1>(
                  [&](const auto& img) {
                    return (double)std::get<0>(
                        img->array()(posXPixRounded, posYPixRounded));
                  });
              labelValue->setText(QString("Value (nearest): %1").arg(value));

            } else {
              labelValue->setText("");
            }
          }
        } else {
          labelPosPixel->setText("");
          labelPosPixelRounded->setText("");
          labelValue->setText("");
        }
      });

  setCurrentStreamImageId(false,
                          QSharedPointer<vx::TomographyRawData2DAccessor>(), "",
                          -1, QJsonValue(), QJsonValue());
}
InfoWidget::~InfoWidget() {}

void InfoWidget::setCurrentStreamImageId(
    bool isValid, const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
    const QString& stream, qint64 imageId, const QJsonValue& metadata,
    const QJsonValue& versionMetadata) {
  // TODO: Show information from metadata?
  (void)metadata;

  if (!isValid) {
    this->currentStream->setText("No image");
    this->currentImageId->setText("");
    this->versionMetadataLabel->setText("");
    this->geometryInfo->setText("");
  } else {
    this->currentStream->setText("Current stream: \"" + stream + "\"");
    this->currentImageId->setText("Current image ID: " +
                                  QString::number(imageId));
    if (!versionMetadata.toObject().contains("Status")) {
      this->versionMetadataLabel->setText("");
    } else {
      auto status = versionMetadata.toObject()["Status"].toObject();
      QString str;
      if (status.contains("Error")) {
        str += "Error: " + status["Error"].toObject()["Name"].toString() +
               ": " + status["Error"].toObject()["Message"].toString();
      }
      if (status.contains("Progress")) {
        double progress = status["Progress"].toDouble();
        str += " " + QString::number(progress * 100, 'f', 1) + "%";
      }
      this->versionMetadataLabel->setText(str);
    }

    QString str;
    try {
      auto geomImages = data->mapStreamImageToGeometryImage(stream, imageId);

      for (const auto& img : geomImages) {
        str += std::get<0>(img);
        str += " / ";
        str += std::get<1>(img)->fullName();
        str += "\n";
      }
    } catch (vx::Exception& e) {
      qWarning() << "Error while geometry images" << e.message();
      str = "";
    }
    geometryInfo->setText(str);
  }
}
