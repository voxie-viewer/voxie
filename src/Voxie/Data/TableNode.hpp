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

#include <Voxie/Data/LabelAttributes.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/Prototypes.forward.hpp>
#include <Voxie/Data/TableColumnListView.hpp>

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/NodePrototype.hpp>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>

#include <QLabel>
#include <QList>
#include <QModelIndexList>
#include <QPushButton>
#include <QTableWidget>

namespace vx {
class TableData;

/**
 * @brief Represents a table structure, organized into columns of arbitrary
 * types, holding an arbitrary number of records.
 */
class VOXIECORESHARED_EXPORT TableNode : public DataNode {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Data.Table")

 public:
  TableNode();

  QSharedPointer<Data> data() override;

  const QSharedPointer<TableData>& tableData() const {
    return tableDataPointer;
  }

  /**
   * @brief Opens a file chooser to save the table's contents to a CSV file.
   */
  void showSaveCsvDialog();

  void saveCsvToFile(QFile& file);

  QSharedPointer<TableData> tableDataPointer;
  TableColumnListView* tableView;
  QLabel* rowCountLabel;
  QPushButton* saveButton;

 protected:
  void setDataImpl(const QSharedPointer<Data>& data) override;

 private:
  void updateTable();
};
}  // namespace vx
