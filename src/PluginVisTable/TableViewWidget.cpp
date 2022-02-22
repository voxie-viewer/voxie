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

#include "TableViewWidget.hpp"

#include <PluginVisTable/TableUtils.hpp>

#include <Voxie/Data/TableData.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QModelIndexList>
#include <QStringList>
#include <QStringListIterator>

using namespace vx;

TableViewWidget::TableViewWidget() : QTableWidget() {
  this->setRowCount(1);
  this->setColumnCount(1);
  this->setHorizontalHeaderLabels(QStringList() << "");

  // Disallow sorting by clicking the header, as it does not make sense to sort
  // by some columns, and Qt does not provide a way to disable sorting on
  // specific columns.
  // TODO: use a custom table model with a sort proxy instead
  this->horizontalHeader()->setSectionsClickable(false);
}

void TableViewWidget::updateTable(QSharedPointer<TableData> tableData) {
  if (tableData == nullptr) {
    clearContents();
    return;
  }

  const auto& columns = tableData->columns();
  auto rows = tableData->getRowsByIndex(0, this->getRowLimit());

  this->setRowCount(rows.size());
  this->setColumnCount(columns.size());

  QList<QString> columnLabels;
  for (const auto& column : columns) {
    columnLabels << TableUtils::getColumnLabel(column);
  }
  this->setHorizontalHeaderLabels(columnLabels);

  // Disable sorting, visual updates and column header resizing while mutating
  // the table for performance
  this->setSortingEnabled(false);
  this->setUpdatesEnabled(false);
  this->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

  this->clearContents();

  for (int i = 0; i < rows.size(); i++) {
    const auto& row = rows[i];
    for (int j = 0; j < columns.size(); j++) {
      const auto& column = tableData->columns()[j];
      auto valRaw = row.data()[j];
      QString str = column.type()->valueToString(valRaw);
      auto item = new SortableItem(str, column.type(), valRaw);
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      this->setItem(i, j, item);
    }
  }

  // Re-enable sorting, visual updates and column header resizing
  this->setSortingEnabled(true);
  this->setUpdatesEnabled(true);
  this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void TableViewWidget::setRowLimit(std::size_t rowLimit) {
  this->rowLimit = rowLimit;
}

std::size_t TableViewWidget::getRowLimit() const { return this->rowLimit; }

TableViewWidget::SortableItem::SortableItem(
    QString text, const QSharedPointer<PropertyType>& type, QVariant sortData)
    : QTableWidgetItem(text),
      type(type),
      sortData(sortData)
// isNaN(sortData != sortData)
{
  setFlags(flags() & ~Qt::ItemIsEditable);
}

bool TableViewWidget::SortableItem::operator<(
    const QTableWidgetItem& other) const {
  if (auto sortableOther = dynamic_cast<const SortableItem*>(&other)) {
    /*
    return isNaN != sortableOther->isNaN ? isNaN > sortableOther->isNaN
                                         : sortData < sortableOther->sortData;
    */
    if (this->type != sortableOther->type) {
      qWarning() << "TableViewWidget::SortableItem::operator<: Got data with "
                    "different types:"
                 << this->type->name() << "and" << sortableOther->type->name();
      return false;
    } else {
      auto type = this->type;
      if (!type->isComparable()) {
        qWarning() << "TableViewWidget::SortableItem::operator<: Called for "
                      "non-comparable type"
                   << type->name();
        return false;
      } else {
        try {
          return type->compare(this->sortData, sortableOther->sortData) < 0;
        } catch (Exception& e) {
          qWarning() << "TableViewWidget::SortableItem::operator<: Error while "
                        "comparing values: "
                     << e.message();
          return false;
        }
      }
    }
  } else {
    qWarning() << "TableViewWidget::SortableItem::operator< called for "
                  "non-SortableItem value";
    return false;
  }
}
