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

#include <QKeyEvent>
#include <QModelIndexList>
#include <QObject>
#include <QTableWidget>

namespace vx {
class TableData;

/**
 * @brief TableViewWidget Displays a TableData object as a text-based table.
 */
class TableViewWidget : public QTableWidget {
  Q_OBJECT

 private:
  class SortableItem : public QTableWidgetItem {
    QSharedPointer<PropertyType> type;
    QVariant sortData;
    // bool isNaN;

   public:
    SortableItem(QString text, const QSharedPointer<PropertyType>& type,
                 QVariant sortData);
    virtual bool operator<(const QTableWidgetItem& other) const override;
  };

 public:
  TableViewWidget();

  /**
   * @brief updateTable Clears the TableViewWidget's contents, and repopulates
   * them from the passed TableData object.
   */
  void updateTable(QSharedPointer<TableData> tableData);

  /**
   * @brief setRowLimit Sets the maximum number of rows to be displayed in this
   * table.
   * @param rowLimit The maximum number of rows to be displayed
   */
  void setRowLimit(std::size_t rowLimit);

  /**
   * @brief getRowLimit Returns the maximum number of rows to be displayed in
   * this table.
   * @return The maximum number of rows to be displayed
   */
  std::size_t getRowLimit() const;

 private:
  std::size_t rowLimit;
};
}  // namespace vx
