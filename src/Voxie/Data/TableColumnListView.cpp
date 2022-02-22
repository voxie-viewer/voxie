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

#include "TableColumnListView.hpp"

#include <Voxie/Data/TableData.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <VoxieClient/DBusUtil.hpp>

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QModelIndexList>
#include <QStringList>
#include <QStringListIterator>

using namespace vx;

TableColumnListView::TableColumnListView() : QTableWidget() {
  QList<QString> columnLabels = {tr("ID"), tr("Name"), tr("Type"), tr("Unit")};
  setColumnCount(columnLabels.size());
  setHorizontalHeaderLabels(columnLabels);

  // Use fixed size for 'unit' column only
  horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
  horizontalHeader()->resizeSection(3, 40);
}

void TableColumnListView::updateTable(const QSharedPointer<TableData>& data) {
  const auto& columns = data->columns();
  clearContents();

  if (data == nullptr) return;

  setRowCount(columns.size());

  for (int index = 0; index < columns.size(); index++) {
    int colIndex = 0;
    auto addEntry = [index, &colIndex, this](QString text) {
      auto item = new QTableWidgetItem(text);
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      setItem(index, colIndex++, item);
    };

    const auto& column = data->columns()[index];

    QString unit = column.metadata().contains("unit")
                       ? column.metadata()["unit"].variant().toString()
                       : "";

    addEntry(column.name());
    addEntry(column.displayName());
    addEntry(column.type()->displayName());
    addEntry(unit);
  }
}
