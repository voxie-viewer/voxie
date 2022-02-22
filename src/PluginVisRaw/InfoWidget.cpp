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

#include "InfoWidget.hpp"

#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>

#include <PluginVisRaw/RawData2DVisualizer.hpp>

#include <QtWidgets/QLabel>

// TODO: Provide information about point under mouse? (See
// src/PluginVisSlice/InfoWidget.cpp)

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
