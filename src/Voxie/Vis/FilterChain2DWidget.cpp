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

#include "FilterChain2DWidget.hpp"

#include <Voxie/IVoxie.hpp>

#include <VoxieBackend/Data/FloatImage.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>

#include <Voxie/OldFilter/Filter2D.hpp>

#include <Voxie/Component/MetaFilter2D.hpp>
#include <Voxie/Component/Plugin.hpp>

#include <QtCore/QDebug>
//#include <QtCore/QMetaMethod>
#include <QtCore/QThreadPool>
#include <QtCore/QVector>

#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

using namespace vx::filter;
using namespace vx;
using namespace vx::visualization;

FilterChain2DWidget::FilterChain2DWidget(QWidget* parent)
    : QWidget(parent), filterchain(new FilterChain2D()) {
  qRegisterMetaType<vx::SliceImage>("vx::SliceImage&");

  QString name = "2D Filters";
  this->setWindowTitle(name);

  // connect(this, &QObject::destroyed, filterchain, &QObject::deleteLater);
  filterchainThread = new QThread();
  filterchain->moveToThread(filterchainThread);
  filterchain->thread()->start();
  connect(this, &QObject::destroyed, this, [this]() {
    filterchain->deleteLater();
    filterchainThread->exit();
    filterchainThread->wait();
    delete filterchainThread;
  });
  // filterchain->setParent(filterchain->thread());

  QVBoxLayout* topLayout = new QVBoxLayout(this);

  //******Toolbar that`s shown over list****

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  QToolBar* toolbar = new QToolBar();

  QAction* addFilterButton =
      toolbar->addAction(QIcon(":/icons/slide--plus.png"), "Add Filter");
  QAction* removeFilterButton =
      toolbar->addAction(QIcon(":/icons/slide--minus.png"), "Remove Filter");
  QAction* moveFilterDownButton =
      toolbar->addAction(QIcon(":/icons/arrow-270.png"), "Filter down");
  QAction* moveFilterUpButton =
      toolbar->addAction(QIcon(":/icons/arrow-090.png"), "Filter up");
  QAction* filterSettingsButton =
      toolbar->addAction(QIcon(":/icons/gear.png"), "Filter Settings");
  QAction* filtermaskButton =
      toolbar->addAction(QIcon(":/icons/mask.png"), "Filter Mask");
  QAction* exportButton =
      toolbar->addAction(QIcon(":/icons/disk.png"), "Export Filterchain");
  QAction* importButton = toolbar->addAction(
      QIcon(":/icons/folder-horizontal-open.png"), "Import Filterchain");

  connect(addFilterButton, &QAction::triggered, this,
          &FilterChain2DWidget::addFilter);
  connect(removeFilterButton, &QAction::triggered, this,
          &FilterChain2DWidget::removeFilter);
  connect(moveFilterDownButton, &QAction::triggered, this,
          &FilterChain2DWidget::moveFilterDown);
  connect(moveFilterUpButton, &QAction::triggered, this,
          &FilterChain2DWidget::moveFilterUp);
  connect(filterSettingsButton, &QAction::triggered, this,
          &FilterChain2DWidget::openSettingsDialog);
  connect(filtermaskButton, &QAction::triggered, this,
          &FilterChain2DWidget::openFiltermaskEditor);
  connect(exportButton, &QAction::triggered, this,
          &FilterChain2DWidget::exportFilterChain);
  connect(importButton, &QAction::triggered, this,
          &FilterChain2DWidget::importFilterChain);

  buttonLayout->addWidget(toolbar);

  //**********List of Filters**************

  QHBoxLayout* filterLayout = new QHBoxLayout();
  list = new QListWidget();
  filterLayout->addWidget(list);

  //*******put all together**********

  topLayout->addLayout(buttonLayout);
  topLayout->addLayout(filterLayout);

  connect(filterchain, &FilterChain2D::filterListChanged, this,
          &FilterChain2DWidget::updateList);
  connect(this->list, &QListWidget::itemClicked, this,
          &FilterChain2DWidget::checkEnabled);

  //******* Dialog is shown when addFilter is clicked ***********
  this->addDialog = new QDialog(this);
  addDialog->setWindowTitle("Add Filter");

  QVBoxLayout* topLayoutDialog = new QVBoxLayout(addDialog);

  // list
  // QHBoxLayout *layout = new QHBoxLayout();
  filterToAdd = new QListWidget();
  topLayoutDialog->addWidget(filterToAdd);
  // topLayoutDialog->addLayout(layout);
  // Ok Buttoon
  QPushButton* okBtn = new QPushButton("Ok");
  connect(okBtn, &QPushButton::clicked, [=]() {
    addDialog->accept();
    // TODO: Probably should use getComponentTyped instead of
    // listComponentsTyped
    for (const auto& metafilter :
         vx::voxieRoot()
             .components()
             ->listComponentsTyped<vx::plugin::MetaFilter2D>()) {
      // TODO: Clean up, don't compare display names
      if (metafilter->displayName().compare(
              this->filterToAdd->currentItem()->text()) == 0) {
        Filter2D* filter = metafilter->createFilter();
        this->filterchain->addFilter(filter);
      }
    }
  });
  topLayoutDialog->addWidget(okBtn);
}

void FilterChain2DWidget::addFilter() {
  this->filterToAdd->clear();  // clears list in the add dialog otherwise therre
                               // are duplicate entries

  for (const auto& metafilter :
       vx::voxieRoot()
           .components()
           ->listComponentsTyped<vx::plugin::MetaFilter2D>()) {
    this->filterToAdd->addItem(metafilter->displayName());
  }
  this->filterToAdd->setCurrentRow(0);
  int dialogResult = addDialog->exec();
  if (dialogResult == QDialog::Accepted) {
    // qDebug() << "add filter";
  }
}

void FilterChain2DWidget::removeFilter() {
  int currentRow = list->currentRow();
  if (currentRow != -1)  // a item must be selected
  {
    this->filterchain->removeFilter(this->filterchain->getFilter(currentRow));
    // emit this->filterListChanged();
  }
}
void FilterChain2DWidget::moveFilterUp() {
  int currentRow = list->currentRow();

  if (currentRow != -1)  // a item must be selected
  {
    if (currentRow == 0) {
      // pos cant be <=1 so do nothing

    } else {
      Filter2D* filter = filterchain->getFilter(currentRow);
      filterchain->changePosition(filter, currentRow - 1);
      list->setCurrentRow(currentRow - 1);  // keep focus on moved item
                                            // emit this->filerListChanged();
    }
  }
}
void FilterChain2DWidget::moveFilterDown() {
  int currentRow = list->currentRow();

  if (currentRow != -1)  // a item must be selected
  {
    if (currentRow + 1 == list->count()) {
      // pos cant be >= items in list. so do nothing

    } else {
      Filter2D* filter = filterchain->getFilter(currentRow);
      filterchain->changePosition(filter, currentRow + 1);
      list->setCurrentRow(currentRow + 1);  // keep focus on moved item
                                            // emit this->filerListChanged();
    }
  }
}

void FilterChain2DWidget::applyFilter(SliceImage slice) {
  // This threads the call by using the thread the object lives in.
  // All direct methods calls are not using the thread the object lives in thus
  // not breaking functionality.
  enqueueOnThread(this->filterchain,
                  [filterchain = this->filterchain, slice]() {
                    filterchain->applyTo(slice);
                  });
}

FilterChain2D* FilterChain2DWidget::getFilterChain() {
  return this->filterchain;
}

void FilterChain2DWidget::updateList() {
  this->list->clear();
  for (int i = 0; i < this->filterchain->getFilters().size(); i++) {
    Filter2D* filter = this->filterchain->getFilter(i);
    // QString name = filter->getName();
    QListWidgetItem* item = new QListWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    if (filter->isEnabled()) {
      item->setCheckState(Qt::Checked);
    } else {
      item->setCheckState(Qt::Unchecked);
    }
    item->setText(filter->metaFilter()->displayName());
    this->list->addItem(item);
    this->list->setCurrentItem(item);
  }
}

void FilterChain2DWidget::checkEnabled(QListWidgetItem* item) {
  int currentRow = this->list->row(item);
  if (item->checkState() == Qt::Unchecked) {
    this->filterchain->getFilter(currentRow)->setEnabled(false);
  }
  if (item->checkState() == Qt::Checked) {
    this->filterchain->getFilter(currentRow)->setEnabled(true);
  }
}

void FilterChain2DWidget::openSettingsDialog() {
  int currentRow = this->list->currentRow();
  if (currentRow != -1)  // a item must be selected
  {
    Filter2D* filter = filterchain->getFilter(currentRow);
    if (filter->hasSettingsDialog()) {
      auto dialog = filter->getSettingsDialog();
      if (dialog != nullptr) {
        dialog->exec();
      }
    }
  }
}

void FilterChain2DWidget::openFiltermaskEditor() {
  int currentRow = this->list->currentRow();
  if (currentRow != -1)  // a item must be selected
  {
    Filter2D* filter = filterchain->getFilter(currentRow);
    Q_EMIT this->requestFilterMaskEditor(filter);
  }
}

void FilterChain2DWidget::exportFilterChain() {
  QString fileName = QFileDialog::getSaveFileName(
      NULL, tr("Export Filterchain"), QDir::currentPath(), "XML file (*.xml)");
  if (fileName.isNull()) {
    return;
  }
  this->filterchain->toXML(fileName);
}

void FilterChain2DWidget::importFilterChain() {
  QString fileName = QFileDialog::getOpenFileName(
      NULL, tr("Import Filterchain"), QDir::currentPath(), "XML file (*.xml)");
  if (fileName.isNull()) {
    return;
  }
  this->filterchain->fromXML(fileName);
}
