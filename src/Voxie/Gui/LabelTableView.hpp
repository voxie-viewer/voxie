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

#pragma once

#include <qlabel.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtableview.h>
#include <qwidget.h>
#include <QHeaderView>
#include <QRadioButton>
#include <QTableView>
#include <QTimer>
#include <QToolBar>
#include <Voxie/Data/LabelViewModel.hpp>

namespace vx {

inline QTableView* setLabelTableView(QTableView* tableView,
                                     LabelViewModel* labelViewModel) {
  QLabel* labelLabel = new QLabel("Labels");
  labelLabel->setStyleSheet("font-weight: bold");

  tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  tableView->setModel(labelViewModel);
  tableView->setFixedHeight(215);
  tableView->setAlternatingRowColors(true);

  tableView->setColumnWidth(
      labelViewModel->getLabelTable()->getColumnIndexByName("Export"), 55);
  tableView->setColumnWidth(
      labelViewModel->getLabelTable()->getColumnIndexByName("LabelID"), 35);
  tableView->setColumnWidth(
      labelViewModel->getLabelTable()->getColumnIndexByName("Visibility"), 55);
  tableView->setColumnWidth(
      labelViewModel->getLabelTable()->getColumnIndexByName("Voxels"), 80);
  tableView->setColumnWidth(
      labelViewModel->getLabelTable()->getColumnIndexByName("Percentage"), 55);
  tableView->horizontalHeader()->setSectionResizeMode(
      labelViewModel->getLabelTable()->getColumnIndexByName("Name"),
      QHeaderView::Stretch);
  tableView->verticalHeader()->hide();

  return tableView;
}
}  // namespace vx
