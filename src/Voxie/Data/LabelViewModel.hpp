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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include <QAbstractListModel>
#include <QColor>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/Interfaces/StepManagerI.hpp>
#include <Voxie/Node/Types.hpp>
#include <VoxieClient/QtUtil.hpp>

namespace vx {

class VOXIECORESHARED_EXPORT LabelViewModel : public QAbstractListModel {
  Q_OBJECT

 private:
  QSharedPointer<TableData> labels;
  StepManagerI* stepManager;

 public:
  LabelViewModel(QSharedPointer<TableData> labels, StepManagerI* stepManager);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;
  bool insertRows(int row, int count,
                  const QModelIndex& parent = QModelIndex()) override;
  bool removeRows(int row, int count,
                  const QModelIndex& parent = QModelIndex()) override;

  int getLabelIDbyRowIdx(int rowIdx);

  /**
   * @brief Find missing label numbers that can be used in our label table
   */
  int getLabelIdCounter() {
    auto freeIds = getLabelIdCounter(1);
    return freeIds[0];
  }

  /**
   * @brief Find multiple missing gap label numbers that can be used in our
   * label table
   * @param numIds number of new IDs to return
   */
  QList<int> getLabelIdCounter(int numIds) {
    QList<int> existingIds = QList<int>();
    for (int i = 0; i < (int)this->labels->rowCount(); i++) {
      existingIds.append(getLabelIDbyRowIdx(i));
    }

    QList<int> freeIds = QList<int>();

    for (int i = 0; i < (existingIds.size() + numIds); i++) {
      if (!existingIds.contains(i + 1)) {
        // get free ids in sequential order
        freeIds.append(i + 1);
        if (freeIds.size() >= numIds) break;
      }
    }
    return freeIds;
  }

  QSharedPointer<TableData> getLabelTable() { return this->labels; }
};

/**
 * @brief Returns the TableRow containing the labelID. Throws an exception if
 * none was found.
 */
inline TableRow getTableRowByLabelID(int labelID,
                                     QSharedPointer<TableData> labelTable) {
  const QList<TableRow> rows = labelTable->getRowsByIndex();
  const auto row = std::find_if(
      rows.begin(), rows.end(), [labelTable, labelID](TableRow const& tr) {
        return tr.data().at(labelTable->getColumnIndexByName("LabelID")) ==
               labelID;
      });
  if (row != rows.end()) {
    return *row;
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        QString("Could not find labelID %1").arg(labelID));
  }
}

}  // namespace vx
