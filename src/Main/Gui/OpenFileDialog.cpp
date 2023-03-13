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

#include "OpenFileDialog.hpp"

#include <VoxieBackend/IO/Importer.hpp>
#include <VoxieBackend/IO/OperationImport.hpp>

#include <VoxieBackend/Property/PropertyBase.hpp>

#include <Voxie/Gui/ErrorMessage.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <Main/Root.hpp>

#include <Main/IO/Load.hpp>

#include <QtCore/QDebug>

using namespace vx;
using namespace vx::io;

OpenFileDialog::OpenFileDialog(Root* root, QWidget* parent)
    : QFileDialog(parent, "Select file to load"), root(root) {
  // qDebug() << "OpenFileDialog::OpenFileDialog()";

  this->importers = *Load::getImporters(root);

  QStringList filters;

  supportedFilter += "All supported files (";
  int i = 0;
  for (auto importer : importers) {
    for (auto pattern : importer->filter().patterns()) {
      if (i != 0) supportedFilter += " ";
      supportedFilter += pattern;
      i++;
    }
  }
  supportedFilter += ")";
  filters << supportedFilter;
  supportedFilter = "All supported files";

  for (auto importer : importers) {
    const QString& filterString = importer->filter().filterString();
    if (map.contains(filterString)) {
      qWarning() << "Got multiple importers with filter string" << filterString;
    } else {
      // filters << filterString;
      // Note: This will add a filter with the list of extensions twice. The
      // second list of extension will be removed by the HideNameFilterDetails
      // option. This is to done in order to hide the list of extensions for the
      // "All supported files" filter (which would be too long).
      filters << importer->filter().filterStringDouble();
      map[filterString] = importer;
    }
  }

  this->setOption(QFileDialog::DontUseNativeDialog, true);
  // Note: This has to be done before calling setNameFilters()
  this->setOption(QFileDialog::HideNameFilterDetails, true);
  this->setFileMode(QFileDialog::ExistingFiles);
  this->setNameFilters(filters);

  // if (this->exec() != QDialog::Accepted) return;

  QObject::connect(this, &QDialog::finished, this, &OpenFileDialog::finish);

  QObject::connect(this, &QFileDialog::filterSelected, this, [this]() {
    vx::enqueueOnThread(this, [this] { updateImporter(); });
  });
  // TODO: This does not seem to be emitted every time the selected file changes
  // (especially with multiple selected files)
  QObject::connect(this, &QFileDialog::currentChanged, this, [this]() {
    // qDebug() << "currentChanged";
    // Make sure selectedFiles() has changed
    vx::enqueueOnThread(this, [this] { updateImporter(); });
  });
  updateImporter();

  this->show();
}

void OpenFileDialog::finish(int result) {
  if (!result) {
    this->deleteLater();
    return;
  }

  for (const auto& file : this->selectedFiles()) {
    QSharedPointer<Importer> importer;
    if (this->selectedNameFilter() == supportedFilter) {
      // qDebug() << "Open" << file;
      // op = Load::openFile(root, file)->operation();
      importer = findImporter(file);
      if (!importer) {
        vx::showErrorMessage("Voxie", "Could not find importer for " + file);
        this->deleteLater();
        return;  // Don't open any more files
      }
    } else {
      importer = map[this->selectedNameFilter()];
      if (!importer) {
        vx::showErrorMessage("Voxie", "Unknown filter");
        this->deleteLater();
        return;
      }
    }
    // qDebug() << "Open" << file << importer;
    QMap<QString, QVariant> properties;
    auto fakeNode = getFakeNode(importer);
    if (fakeNode) {
      for (const auto& prop : importer->properties())
        properties[prop->name()] = fakeNode->getNodeProperty(prop->name());
    }
    auto op = Load::openFile(root, importer, file, properties)->operation();

    op->onFinishedError(
        root, [](const QSharedPointer<Operation::ResultError>& opResult) {
          vx::showErrorMessage("Error while opening file", *opResult->error());
        });
  }

  this->deleteLater();
}
OpenFileDialog::~OpenFileDialog() {
  // qDebug() << "OpenFileDialog::~OpenFileDialog()";
}

QSharedPointer<vx::io::Importer> OpenFileDialog::findImporter(
    const QString& filename) {
  for (const auto& importer : this->importers) {
    // qDebug() << filename << importer << importer->filter().matches(filename);
    if (importer->filter().matches(filename)) return importer;
  }
  return QSharedPointer<vx::io::Importer>();
}

void OpenFileDialog::updateImporter() {
  auto filter = this->selectedNameFilter();

  // qDebug() << "updateImporter" << this->selectedFiles() << (filter !=
  // supportedFilter);

  if (filter != supportedFilter) {
    auto importer = map[filter];
    if (!importer) {
      qWarning() << "Unknown filter";
      return;
    }
    setImporter(importer);
    return;
  }

  QSharedPointer<vx::io::Importer> importer;
  for (const auto& file : this->selectedFiles()) {
    auto imp = findImporter(file);

    if (importer == imp) continue;

    if (importer) {
      setImporter(QSharedPointer<vx::io::Importer>());
      return;
    }

    importer = imp;
  }
  setImporter(importer);
}

QSharedPointer<vx::Node> OpenFileDialog::getFakeNode(
    const QSharedPointer<vx::io::Importer>& importer) {
  if (!importer) return QSharedPointer<vx::Node>();
  if (importer->properties().length() == 0) return QSharedPointer<vx::Node>();
  if (fakeNodes.contains(importer)) return fakeNodes[importer];

  auto prototype =
      Root::instance()->propertyFakeNodes[importer->properties()[0]];
  if (!prototype) {
    qCritical() << "Could not find prototype in propertyFakeNodes";
    return QSharedPointer<vx::Node>();
  }

  auto obj = prototype->create({}, {}, {});
  fakeNodes[importer] = obj;
  return obj;
}

void OpenFileDialog::setImporter(
    const QSharedPointer<vx::io::Importer>& importer) {
  // qDebug() << "setImporter" << importer;

  if (importer == currentImporter) return;

  // qDebug() << "OpenFileDialog: Change from" << currentImporter << "to"
  //          << importer;
  currentImporter = importer;

  auto fakeNode = getFakeNode(importer);

  auto layout = dynamic_cast<QGridLayout*>(this->layout());
  if (!layout) {
    qWarning() << "QFileDialog does not have a QGridLayout as layout";
    return;
  }

  for (const auto& widget : additionalWidgets) {
    layout->removeWidget(widget);
    // widget->deleteLater();
    widget->hide();
  }
  additionalWidgets.clear();

  if (!fakeNode) return;

  auto sections = fakeNode->propertySections();
  if (sections.length() != 2) {
    qWarning() << "FakeFilter does not have 2 property sections";
    return;
  }
  auto section = sections[1];
  // auto pLayout = section->layout();
  // qDebug() << pLayout;

  // TODO: Insert this in a nicer way

  // qDebug() << layout->rowCount() << layout->columnCount();

  // int row = layout->rowCount();
  int row = 4;  // TODO
  auto widget = section;
  additionalWidgets << widget;
  layout->addWidget(widget, row, 0, 1, layout->columnCount() - 1);
  widget->show();
  row++;
}
