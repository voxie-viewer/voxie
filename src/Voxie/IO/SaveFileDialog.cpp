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

#include "SaveFileDialog.hpp"

#include <QtCore/QDebug>

using namespace vx::io;

SaveFileDialog::SaveFileDialog(QWidget* parent, const QString& caption,
                               const QString& directory)
    : QFileDialog(parent, caption, directory) {
  setAcceptMode(QFileDialog::AcceptSave);
  setFileMode(QFileDialog::AnyFile);
  setOption(QFileDialog::DontUseNativeDialog, true);

  // qDebug() << "SaveFileDialog::SaveFileDialog()";
}
SaveFileDialog::~SaveFileDialog() {
  // qDebug() << "SaveFileDialog::~SaveFileDialog()";
}

const SaveFileDialog::Filter* SaveFileDialog::currentFilter() {
  QString str = selectedNameFilter();
  for (int i = 0; i < filters.size(); i++)
    if (filters[i].filter.filterString() == str) return &filters[i];

  qWarning() << "SaveFileDialog: Could not find filter:" << str;
  return nullptr;
}

void SaveFileDialog::addFilter(const FilenameFilter& filter,
                               const std::shared_ptr<void>& data,
                               bool isDefault) {
  Filter filterData;
  filterData.filter = filter;
  filterData.data = data;
  filterData.isDefault = isDefault;

  if (filterData.filter.patterns().size() == 0) {
    qCritical() << "Got a filter without any patterns";
    return;
  }

  filters.push_back(filterData);
}

void SaveFileDialog::setup() {
  QStringList filterList;
  QString defaultFilterString = "";
  for (int i = 0; i < filters.size(); i++) {
    filterList << filters[i].filter.filterString();
    if (filters[i].isDefault)
      defaultFilterString = filters[i].filter.filterString();
  }
  setNameFilters(filterList);
  if (defaultFilterString != "") selectNameFilter(defaultFilterString);
}

void SaveFileDialog::accept() {
  if (selectedFiles().size() == 0) {
    QFileDialog::accept();
    return;
  }
  QString file = selectedFiles()[0];

  if (QFileInfo(file).isDir()) {
    QFileDialog::accept();
    return;
  }

  for (const auto& filter : filters) {
    if (filter.filter.matches(file)) {
      selectNameFilter(filter.filter.filterString());
      QFileDialog::accept();
      return;
    }
  }
  for (const auto& filter : filters) {
    if (filter.filter.matchesCaseInsensitive(file)) {
      selectNameFilter(filter.filter.filterString());
      QFileDialog::accept();
      return;
    }
  }

  const Filter* filter = currentFilter();
  if (filter && filter->filter.patterns().size()) {
    file = filter->filter.forceMatch(file);
    this->setFocus(Qt::OtherFocusReason);  // Make lineEdit loose focus,
                                           // otherwise if the lineEdit is
                                           // selected it will not be updated
    this->selectFile(file);
  }
  QFileDialog::accept();
}

std::shared_ptr<void> SaveFileDialog::selectedFilterData() {
  const Filter* filter = currentFilter();
  if (!filter) return nullptr;
  return filter->data;
}
