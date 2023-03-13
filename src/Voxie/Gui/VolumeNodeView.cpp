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
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "VolumeNodeView.hpp"

#include <Voxie/Data/Slice.hpp>

#include <Voxie/IVoxie.hpp>

#include <Voxie/Component/Plugin.hpp>

#include <QtWidgets/QAction>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

using namespace vx::gui;
using namespace vx;
using namespace vx::plugin;
using namespace vx::io;
using namespace vx::filter;

VolumeNodeView::VolumeNodeView(vx::VolumeNode* dataSet, QWidget* parent)
    : QWidget(parent), sequenceNumber(0), dataSet(dataSet) {
  this->setMaximumHeight(400);
  splitLayout = new QVBoxLayout();

  form = new QFormLayout();

  auto filename = dataSet->getFileInfo().fileName();
  this->filenameLabel = new QLineEdit(filename);
  // this->filenameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse |
  //                                             Qt::TextSelectableByKeyboard);
  this->filenameLabel->setReadOnly(true);
  form->addRow("Filename", this->filenameLabel);
  QObject::connect(dataSet, &DataNode::fileInfoChanged, this,
                   [this](const QFileInfo& fileInfo) {
                     this->filenameLabel->setText(fileInfo.filePath());
                   });

  tagLabel = new QLabel();
  form->addRow("Tags", tagLabel);

  splitLayout->addLayout(form);

  this->setLayout(splitLayout);
  this->setWindowTitle(this->dataSet->displayName());
  this->update();
  connect(this->dataSet, &Node::displayNameChanged, this,
          &VolumeNodeView::setWindowTitle);

  connect(this->dataSet, &VolumeNode::changed, this, &VolumeNodeView::update);

  QMetaObject::Connection conni =
      connect(this->dataSet, &QObject::destroyed, [this]() -> void {
        this->dataSet = nullptr;
        this->deleteLater();
      });
  connect(this, &QObject::destroyed,
          [=]() -> void { this->disconnect(conni); });
}

void VolumeNodeView::setValues(
    const QList<std::tuple<QString, QString>>& data) {
  bool change = false;
  if (labels.size() != data.size()) {
    change = true;
  } else {
    for (int i = 0; i < labels.size(); i++) {
      if (std::get<0>(labels[i]) != std::get<0>(data[i])) {
        change = true;
        break;
      }
    }
  }

  if (change) {
    this->filenameLabel->setParent(nullptr);
    this->tagLabel->setParent(nullptr);

    // https://doc.qt.io/qt-5/qlayout.html#takeAt
    QLayoutItem* child;
    // Add check for form->count() to avoid
    // "QFormLayout::takeAt: Invalid index 0" warning
    while (form->count() && (child = form->takeAt(0)) != nullptr) {
      delete child->widget();  // delete the widget
      delete child;            // delete the layout item
    }

    labels.clear();

    form->addRow("Filename", this->filenameLabel);

    for (int i = 0; i < data.size(); i++) {
      auto name = std::get<0>(data[i]);

      auto label = new QLabel();
      label->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                     Qt::TextSelectableByKeyboard);
      form->addRow(name, label);

      labels.push_back(std::make_tuple(name, label));
    }

    form->addRow("Tags", this->tagLabel);
  }

  for (int i = 0; i < data.size(); i++) {
    auto label = std::get<1>(labels[i]);
    auto value = std::get<1>(data[i]);
    label->setText(value);
  }
}

void VolumeNodeView::update() {
  // int precision = 6; // Takes too much space
  int precision = 5;

  QList<std::tuple<QString, QString>> fields;

  auto data = this->dataSet ? this->dataSet->volumeData()
                            : QSharedPointer<vx::VolumeData>();
  if (!data) {
    fields << std::make_tuple("Size", "");
    fields << std::make_tuple("Origin", "");
  } else {
    fields << std::make_tuple(
        "Size", QString::number(data->volumeSize().x()) + " x " +
                    QString::number(data->volumeSize().y()) + " x " +
                    QString::number(data->volumeSize().z()));

    fields << std::make_tuple(
        "Origin", QString::number(data->origin().x(), 'g', precision) + " x " +
                      QString::number(data->origin().y(), 'g', precision) +
                      " x " +
                      QString::number(data->origin().z(), 'g', precision));

    data->getVolumeInfo(fields);
  }

  auto dataObj = dynamic_cast<DataNode*>(dataSet);

  QList<QSharedPointer<NodeTag>> tagList;
  if (dataObj) tagList = dataObj->getTags();
  QString tooltip = "";
  for (QSharedPointer<NodeTag> tag : tagList) {
    tooltip.append("Name: " + tag->getName() + "\n");
    tooltip.append("Description: " + tag->getDescription() + "\n\n");
  }
  tooltip.chop(2);
  this->tagLabel->setText(vx::NodeTag::joinDisplayNames(tagList, ", "));
  this->tagLabel->setToolTip(tooltip);

  setValues(fields);
}

VolumeNodeView::~VolumeNodeView() {
  if (this->dataSet != nullptr) this->dataSet->deleteLater();
}
