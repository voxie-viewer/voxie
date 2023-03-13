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

#include "DataNodeUI.hpp"

#include <Voxie/Component/Plugin.hpp>

#include <Voxie/Gui/ErrorMessage.hpp>

#include <Voxie/Node/DataNode.hpp>

#include <Voxie/IO/SaveFileDialog.hpp>

#include <VoxieBackend/Data/SharedMemory.hpp>

#include <VoxieBackend/IO/Exporter.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieBackend/Component/Extension.hpp>
#include <VoxieBackend/Component/ExtensionExporter.hpp>

#include <Main/Root.hpp>

#include <QtCore/QPointer>

#include <QtWidgets/QPushButton>

using namespace vx;
using namespace vx::io;

void vx::showExportUI(vx::DataNode* obj) {
  // qDebug() << "Export" << obj;

  QList<QSharedPointer<vx::io::Exporter>> exporters;
  for (const auto& exporter : vx::Root::instance()
                                  ->allComponents()
                                  ->listComponentsTyped<vx::io::Exporter>()) {
    // qDebug() << "exp" << exporter;
    if (!exporter->matches(obj->prototype()->name())) continue;

    // qDebug() << "Got exporter for" << obj << ":" << exporter;
    exporters << exporter;
  }

  auto defaultExporter = obj->defaultExporter();

  if (exporters.count() == 0) {
    qWarning() << "vx::showExportUI(): No exporters found";
    return;
  }

  typedef QSharedPointer<vx::io::Exporter> ExpType;

  auto dialog = makeSharedQObject<vx::io::SaveFileDialog>(
      voxieRoot().mainWindow(), QObject::tr("Select target file"), QString());
  QWeakPointer<vx::io::SaveFileDialog> dialogWeak = dialog;

  for (auto exporter : exporters) {
    auto filter = exporter->filter();
    dialog->addFilter(filter, std::make_shared<ExpType>(exporter),
                      exporter == defaultExporter);
  }

  dialog->setup();
  // if (!dialog->exec()) return;
  QPointer<vx::DataNode> objWeak =
      obj;  // TODO: This should use QWeakPointer instead
  // strongRef keeps a reference while the dialog is being shown
  auto strongRef =
      createQSharedPointer<QSharedPointer<vx::io::SaveFileDialog>>();
  QObject::connect(
      dialog.data(), &QDialog::finished,
      [dialogWeak, strongRef, objWeak](int result) {
        // qDebug() << "finished" << result;
        auto dialog2 = dialogWeak.lock();
        strongRef->reset();
        if (!result) return;

        try {
          if (!dialog2)
            throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                "Dialog is deleted");

          auto data = dialog2->selectedFilterData();
          if (!data)
            throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                "No exporter selected");
          auto exporter = *std::static_pointer_cast<ExpType>(data);

          vx::DataNode* obj2 = objWeak;
          if (!obj2)
            throw vx::Exception(
                "de.uni_stuttgart.Voxie.Error",
                "Node was deleted while the export dialog was open");

          auto opRes = exporter->exportData(obj2->data(),
                                            dialog2->selectedFiles().first());
          opRes->onFinishedError(
              Root::instance(),
              [](const QSharedPointer<Operation::ResultError>& opResult) {
                vx::showErrorMessage("Error during export",
                                     opResult->error()->message());
              });
        } catch (vx::Exception& e) {
          vx::showErrorMessage("Error while starting export", e);
        }
      });
  *strongRef = dialog;
  dialog->show();
}

static QString bytesToString(quint64 bytes) {
  // return QString::number(size) + " bytes";
  if (bytes >= 1024ULL * 1024 * 1024 * 1024) {
    return QString::number(bytes / (1024.0 * 1024 * 1024 * 1024), 'f', 1) +
           "TiB";
  } else if (bytes >= 1024ULL * 1024 * 1024) {
    return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 1) + "GiB";
  } else if (bytes >= 1024ULL * 1024) {
    return QString::number(bytes / (1024.0 * 1024), 'f', 1) + "MiB";
  } else if (bytes >= 1024ULL) {
    return QString::number(bytes / (1024.0), 'f', 1) + "kiB";
  } else {
    return QString::number(bytes) + "B";
  }
}

void vx::createDataNodeUI(vx::DataNode* obj) {
  // qDebug() << "createDataNodeUI" << obj;
  QList<QSharedPointer<vx::io::Exporter>> exporters;
  for (const auto& exporter : vx::Root::instance()
                                  ->allComponents()
                                  ->listComponentsTyped<vx::io::Exporter>()) {
    // qDebug() << "exp" << exporter;
    if (!exporter->matches(obj->prototype()->name())) continue;

    // qDebug() << "Got exporter for" << obj << ":" << exporter;
    exporters << exporter;
  }

  auto widget = new QWidget();
  widget->setWindowTitle("Data");
  obj->addPropertySection(widget);

  auto layout = new QVBoxLayout();
  widget->setLayout(layout);

  if (exporters.count() != 0) {
    auto exportButton =
        new QPushButton(QIcon(":/icons/disk.png"), QObject::tr("Export"));
    layout->addWidget(exportButton);
    QObject::connect(exportButton, &QPushButton::clicked, obj,
                     [obj]() { vx::showExportUI(obj); });

    // Add context menu action
    auto exportAction = new QAction("&Export", obj);
    QObject::connect(exportAction, &QAction::triggered, obj,
                     [obj]() { vx::showExportUI(obj); });
    obj->addContextMenuAction(exportAction);
  }

  auto sizeLabel = new QLabel();
  sizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                     Qt::TextSelectableByKeyboard);
  layout->addWidget(sizeLabel);
  enqueueOnMainThread([sizeLabelPtr = QPointer<QLabel>(sizeLabel),
                       objPtr = QPointer<vx::DataNode>(obj)]() {
    auto onDataChange = [sizeLabelPtr](
                            const QSharedPointer<Data>& data,
                            const QSharedPointer<DataVersion>& newVersion,
                            DataChangedReason reason) {
      (void)newVersion;
      (void)reason;

      QList<QSharedPointer<SharedMemory>> sharedMemSections;
      if (data) sharedMemSections = data->getSharedMemorySections();
      QString text = "";
      if (sharedMemSections.count() != 0) {
        quint64 size = 0;
        for (const auto& section : sharedMemSections)
          size += section->getSizeBytes();
        text = "In-memory size: " + bytesToString(size);
      }
      if (sizeLabelPtr)
        sizeLabelPtr->setText(text);
      else
        qWarning() << "vx::createDataNodeUI() onDataChange(): Label is "
                      "destroyed";
    };
    if (!objPtr) {
      qWarning() << "vx::createDataNodeUI(): DataNode already "
                    "destroyed when initializing";
      return;
    }
    QObject::connect(objPtr, &DataNode::dataChangedFinished, objPtr,
                     onDataChange);
    auto data = objPtr->data();
    onDataChange(data,
                 data ? data->currentVersion() : QSharedPointer<DataVersion>(),
                 DataChangedReason::Initialized);
  });
}
