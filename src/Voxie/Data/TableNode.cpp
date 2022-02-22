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

#include "TableNode.hpp"

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/TableData.hpp>

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QModelIndex>
#include <QModelIndexList>
#include <QPushButton>
#include <QStringListIterator>
#include <QTableWidgetItem>
#include <QTextStream>

using namespace vx;

TableNode::TableNode()
    : DataNode("TableNode", getPrototypeSingleton()),
      properties(new PropertiesType(this)),
      tableDataPointer(TableData::create()) {
  setAutomaticDisplayName(tr("Table"));

  PropertySection* columnSection = new PropertySection(tr("Columns"));
  addPropertySection(columnSection);

  tableView = new TableColumnListView;
  columnSection->addProperty(tableView);

  PropertySection* tableSection = new PropertySection(tr("Table"));
  addPropertySection(tableSection);

  rowCountLabel = new QLabel;
  tableSection->addProperty(rowCountLabel);

  connect(this, &DataNode::dataChangedFinished, this, &TableNode::updateTable);
  updateTable();
}

QSharedPointer<Data> TableNode::data() { return tableDataPointer; }

void TableNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<TableData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a TableData object");
  tableDataPointer = dataCast;
}

void TableNode::updateTable() {
  if (tableData() != nullptr) {
    rowCountLabel->setText(tr("Row count: %1").arg(tableData()->rowCount()));
  } else {
    rowCountLabel->setText(tr("Row count: 0"));
  }
  tableView->updateTable(tableData());
}

NODE_PROTOTYPE_IMPL_2(Table, Node)
