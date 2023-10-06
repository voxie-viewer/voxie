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

#include "TomographyRawDataNodeView.hpp"

#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>

#include <VoxieClient/JsonDBus.hpp>

#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

using namespace vx::gui;

// TODO: Use dbusGetVariantValue() etc. and avoid using QVariant?

class JsonInfoDialog : public QDialog {
 public:
  JsonInfoDialog(const QString& data) : QDialog(vx::voxieRoot().mainWindow()) {
    this->resize(500 / 96.0 * this->logicalDpiX(),
                 450 / 96.0 * this->logicalDpiY());
    QVBoxLayout* layout = new QVBoxLayout();
    this->setLayout(layout);

    auto edit = new QPlainTextEdit(data, this);
    layout->addWidget(edit);
    edit->setReadOnly(true);
  }
  ~JsonInfoDialog() {
    // qDebug() << "~JsonInfoDialog()";
  }
};

TomographyRawDataNodeView::TomographyRawDataNodeView(
    vx::TomographyRawDataNode* rawObject, QWidget* parent)
    : QWidget(parent), rawObject(rawObject) {
  this->setWindowTitle("Tomography raw data");
  this->setMaximumHeight(400 / 96.0 * this->logicalDpiY());
  QVBoxLayout* splitLayout = new QVBoxLayout();

  QFormLayout* form = new QFormLayout();

  form->addRow("Dimension", this->dimension = new QLabel());

  form->addRow("Number of Images", this->numberOfImages = new QLabel());
  form->addRow("Distance Source-Axis",
               this->distanceSourceAxis = new QLabel());
  form->addRow("Distance Source-Detector",
               this->distanceSourceDetector = new QLabel());

  form->addRow("Detector pixel size", this->detectorPixelSize = new QLabel());

  // TODO: available image kinds?
  form->addRow("Available streams", this->availableStreams = new QLabel());
  form->addRow("Available geometry types",
               this->availableGeometryTypes = new QLabel());

  showMetadata = new QPushButton("Show metadata");
  form->addRow("", showMetadata);
  QObject::connect(showMetadata, &QPushButton::clicked, this, [this]() {
    auto data = this->rawObject->dataAccessor();
    if (!data) return;
    QString result;
    try {
      auto metadata = data->metadata();
      result = QJsonDocument(metadata).toJson();
    } catch (vx::Exception& e) {
      result = e.message();
    }
    auto dialog = new JsonInfoDialog(result);
    dialog->show();
    QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
  });

  this->geometrySelect = new QComboBox();
  this->showGeometry = new QPushButton("Show geometry");
  form->addRow(geometrySelect, showGeometry);
  QObject::connect(showGeometry, &QPushButton::clicked, this, [this]() {
    auto data = this->rawObject->dataAccessor();
    if (!data) return;
    QString result;
    try {
      auto geometry =
          data->getGeometryData(this->geometrySelect->currentText());
      result = QJsonDocument(geometry).toJson();
    } catch (vx::Exception& e) {
      result = e.message();
    }
    auto dialog = new JsonInfoDialog(result);
    dialog->show();
    QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
  });

  splitLayout->addItem(form);

  this->setLayout(splitLayout);

  connect(rawObject, &vx::DataNode::dataChangedFinished, this,
          &TomographyRawDataNodeView::update);
  this->update();
}

static QList<QString> quoteStrings(const QList<QString>& list) {
  QList<QString> result;
  for (const auto& s : list) result << "\"" + s + "\"";
  return result;
}

void TomographyRawDataNodeView::update() {
  auto data = this->rawObject->dataAccessor();

  if (!data) {
    this->dimension->setText("-");

    this->numberOfImages->setText("-");
    this->distanceSourceAxis->setText("-");
    this->distanceSourceDetector->setText("-");

    this->detectorPixelSize->setText("-");

    this->availableStreams->setText("-");
    this->availableGeometryTypes->setText("-");

    this->showMetadata->setEnabled(false);
    this->geometrySelect->clear();
    this->geometrySelect->setEnabled(false);
    this->showGeometry->setEnabled(false);

    return;
  }

  bool haveDetectorPixelSize = false;
  bool haveDistanceSourceAxis = false;
  bool haveDistanceSourceDetector = false;
  double detectorPixelSizeX = -1;
  double detectorPixelSizeY = -1;
  double distanceSourceAxis = -1;
  double distanceSourceDetector = -1;
  auto metadata = data->metadata();
  if (metadata.contains("Info")) {
    auto info = metadata["Info"].toObject();
    if (info.contains("DetectorPixelSize")) {
      auto detectorPixelSize = info["DetectorPixelSize"].toArray();
      if (detectorPixelSize.size() == 2) {
        detectorPixelSizeX = detectorPixelSize[0].toDouble();
        detectorPixelSizeY = detectorPixelSize[1].toDouble();
        haveDetectorPixelSize = true;
      }
    }
    // if (info.contains("GeometryType") && info["GeometryType"].toString() ==
    // "ConeBeamCT") {

    // TODO: Use information from GetGeometryData("ConeBeamCT") instead of
    // GetMetadata()?

    if (info.contains("Geometry")) {
      const auto& geometry = info["Geometry"].toObject();
      if (geometry.contains("DistanceSourceAxis")) {
        distanceSourceAxis = geometry["DistanceSourceAxis"].toDouble();
        haveDistanceSourceAxis = true;
      }
      if (geometry.contains("DistanceSourceDetector")) {
        distanceSourceDetector = geometry["DistanceSourceDetector"].toDouble();
        haveDistanceSourceDetector = true;
      }
    }
  }

  if (!data->hasStream("") || !data->numberOfImages("")) {
    // No images
    this->dimension->setText("- x -");
  } else {
    auto size0 = data->imageSize("", 0);

    this->dimension->setText(QString::number(size0.x) + " x " +
                             QString::number(size0.y));
  }

  if (!data->hasStream(""))
    this->numberOfImages->setText("-");
  else
    this->numberOfImages->setText(QString::number(data->numberOfImages("")));

  if (haveDistanceSourceAxis)
    this->distanceSourceAxis->setText(QString::number(distanceSourceAxis));
  else
    this->distanceSourceAxis->setText("-");
  if (haveDistanceSourceDetector)
    this->distanceSourceDetector->setText(
        QString::number(distanceSourceDetector));
  else
    this->distanceSourceDetector->setText("-");

  if (haveDetectorPixelSize)
    this->detectorPixelSize->setText(QString::number(detectorPixelSizeX) +
                                     " x " +
                                     QString::number(detectorPixelSizeY));
  else
    this->detectorPixelSize->setText("- x -");

  this->availableStreams->setText(
      quoteStrings(data->availableStreams()).join("\n"));
  this->availableGeometryTypes->setText(
      quoteStrings(data->availableGeometryTypes()).join("\n"));

  this->showMetadata->setEnabled(true);

  this->geometrySelect->clear();
  for (const auto& type : data->availableGeometryTypes())
    this->geometrySelect->addItem(type);
  this->geometrySelect->setEnabled(true);
  this->showGeometry->setEnabled(true);
}
