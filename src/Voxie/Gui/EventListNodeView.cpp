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

#include "EventListNodeView.hpp"

#include <Voxie/Data/EventListNode.hpp>

#include <VoxieBackend/Data/EventListDataAccessor.hpp>

#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/JsonDBus.hpp>

#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

using namespace vx;

// TODO: Merge with TomographyRawDataNodeView
namespace {
class JsonInfoDialog : public QDialog {
 public:
  JsonInfoDialog(const QString& data) {
    this->resize(500, 450);
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
}  // namespace

EventListNodeView::EventListNodeView(vx::EventListNode* node) : node(node) {
  this->setMaximumHeight(400);
  QVBoxLayout* splitLayout = new QVBoxLayout();

  QFormLayout* form = new QFormLayout();

  form->addRow("Number of Streams: ", this->numberOfStreams = new QLabel());

  showMetadata = new QPushButton("Show metadata");
  form->addRow("", showMetadata);
  QObject::connect(showMetadata, &QPushButton::clicked, this, [this]() {
    auto data = this->node->dataAccessor();
    if (!data) return;
    vx::enqueueOnBackgroundThread([data] {
      QString result;
      try {
        auto metadata = data->getMetadata(QMap<QString, QDBusVariant>());
        // TODO: Should getMetadata() return a QDBusVariant?
        // auto metadataJson = vx::dbusToJson(metadata);

        // This won't work because the created QDBusVariant cannot be read by
        // dbusGetVariantValue():
        // auto metadata2 =
        // vx::dbusMakeVariant<QMap<QString,QDBusVariant>>(metadata);
        // auto metadataJson = vx::dbusToJson(metadata2);
        // result = QJsonDocument(metadataJson.toObject()).toJson();

        auto metadataJson = vx::dbusToJson(metadata);
        result = QJsonDocument(metadataJson).toJson();
      } catch (vx::Exception& e) {
        result = e.message();
      }
      enqueueOnMainThread([result] {
        auto dialog = new JsonInfoDialog(result);
        dialog->show();
        QObject::connect(dialog, &QDialog::finished, dialog,
                         &QObject::deleteLater);
      });
    });
  });

  this->streamSelect = new QSpinBox();
  this->showStreamInfo = new QPushButton("Show stream info");
  form->addRow(streamSelect, showStreamInfo);
  QObject::connect(showStreamInfo, &QPushButton::clicked, this, [this]() {
    auto data = this->node->dataAccessor();
    if (!data) return;
    quint64 streamID = this->streamSelect->value();
    vx::enqueueOnBackgroundThread([data, streamID] {
      QString result;
      try {
        QMap<QString, QDBusVariant> options;
        qint64 minimumTimestamp;
        qint64 maximumTimestamp;
        QList<std::tuple<QString, std::tuple<QString, quint32, QString>,
                         QString, QMap<QString, QDBusVariant>,
                         QMap<QString, QDBusVariant>>>
            attributes;
        QMap<QString, QDBusVariant> metadata;
        data->getStreamInfo(streamID, options, minimumTimestamp,
                            maximumTimestamp, attributes, metadata);

        QJsonArray attributesJson;
        for (const auto& attribute : attributes)
          attributesJson << QJsonObject{
              {"Name", std::get<0>(attribute)},
              {"Type", QJsonArray{std::get<0>(std::get<1>(attribute)),
                                  (qint64)std::get<1>(std::get<1>(attribute)),
                                  std::get<2>(std::get<1>(attribute))}},
              {"DisplayName", std::get<2>(attribute)},
              {"Metadata", vx::dbusToJson(std::get<3>(attribute))},
              {"Options", vx::dbusToJson(std::get<4>(
                              attribute))},  // TODO: Should this work?
          };
        QJsonObject info{
            {"StreamID", (qint64)streamID},
            {"Options", vx::dbusToJson(options)},  // TODO: Should this work?
            {"MinimumTimestamp", minimumTimestamp},
            {"MaximumTimestamp", maximumTimestamp},
            {"Attributes", attributesJson},
            {"Metadata", vx::dbusToJson(metadata)},
        };

        result = QJsonDocument(info).toJson();
      } catch (vx::Exception& e) {
        result = e.message();
      }
      enqueueOnMainThread([result] {
        auto dialog = new JsonInfoDialog(result);
        dialog->show();
        QObject::connect(dialog, &QDialog::finished, dialog,
                         &QObject::deleteLater);
      });
    });
  });

  splitLayout->addItem(form);

  this->setLayout(splitLayout);

  connect(node, &vx::DataNode::dataChangedFinished, this,
          &EventListNodeView::update);
  this->update();
}

void EventListNodeView::update() {
  auto data = this->node->dataAccessor();

  if (!data) {
    this->numberOfStreams->setText("-");

    this->showMetadata->setEnabled(false);
    this->streamSelect->setEnabled(false);
    this->showStreamInfo->setEnabled(false);

    return;
  }

  auto streamCount = data->getStreamCount();

  this->numberOfStreams->setText(QString::number(streamCount));

  this->showMetadata->setEnabled(true);

  if (!streamCount) {
    this->streamSelect->setValue(0);
    this->streamSelect->setEnabled(false);
    this->showStreamInfo->setEnabled(false);
  } else {
    this->streamSelect->setRange(0, streamCount - 1);
    this->streamSelect->setEnabled(true);
    this->showStreamInfo->setEnabled(true);
  }
}
