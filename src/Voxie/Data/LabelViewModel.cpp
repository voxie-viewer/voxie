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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include <Voxie/Data/Color.hpp>
#include "LabelViewModel.hpp"

namespace vx {

LabelViewModel::LabelViewModel(QSharedPointer<TableData> labels,
                               StepManagerI* stepManager)
    : labels(labels), stepManager(stepManager) {
  connect(labels.data(), &TableData::dataChanged, this, [this]() {
    Q_EMIT layoutChanged();
    Q_EMIT dataChanged(this->index(0), this->index(this->rowCount() - 1));
  });
}

int LabelViewModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return this->labels->rowCount();
}

int LabelViewModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  // Subtract 2 because two of the columns (Color and Description) are not
  // displayed as columns in the GUI
  return this->labels->columns().size() - 2;
}

QVariant LabelViewModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  if ((uint32_t)index.row() >= this->labels->rowCount()) return QVariant();

  // Text alignment
  if (role == Qt::TextAlignmentRole) {
    if (index.column() == labels->getColumnIndexByName("LabelID") ||
        index.column() == labels->getColumnIndexByName("Voxels") ||
        index.column() == labels->getColumnIndexByName("Percent")) {
      return Qt::AlignVCenter + Qt::AlignRight;
    } else if (index.column() == labels->getColumnIndexByName("Visibility")) {
      return Qt::AlignVCenter + Qt::AlignHCenter;
    } else if (index.column() == labels->getColumnIndexByName("Export")) {
      return Qt::AlignVCenter + Qt::AlignHCenter;
    } else {
      return Qt::AlignVCenter + Qt::AlignLeft;
    }
  }

  // LabelID
  if (role == Qt::DisplayRole &&
      index.column() == labels->getColumnIndexByName("LabelID")) {
    return this->labels->getRowColumnData(index.row(), "LabelID");
  }

  // Color
  if (role == Qt::DecorationRole &&
      index.column() == labels->getColumnIndexByName("Name")) {
    return Color(this->labels->getRowColumnData(index.row(), "Color")
                     .value<TupleVector<double, 4>>())
        .asQColor();
  }

  // Label name
  if ((role == Qt::DisplayRole || role == Qt::EditRole) &&
      index.column() == labels->getColumnIndexByName("Name")) {
    return this->labels->getRowColumnData(index.row(), "Name");
  }

  // Visibility
  if (role == Qt::CheckStateRole &&
      index.column() == labels->getColumnIndexByName("Visibility")) {
    return this->labels->getRowColumnData(index.row(), "Visibility").toBool()
               ? Qt::CheckState::Checked
               : Qt::CheckState::Unchecked;
  }

  // Export
  if (role == Qt::CheckStateRole &&
      index.column() == labels->getColumnIndexByName("Export")) {
    return this->labels->getRowColumnData(index.row(), "Export").toBool()
               ? Qt::CheckState::Checked
               : Qt::CheckState::Unchecked;
  }

  // Voxel count
  if (role == Qt::DisplayRole &&
      index.column() == labels->getColumnIndexByName("Voxels")) {
    return this->labels->getRowsByIndex(index.row(), index.row() + 1)
        .first()
        .data()
        .at(labels->getColumnIndexByName("Voxels"));
  }

  // Voxel count Percent
  if (role == Qt::DisplayRole &&
      index.column() == labels->getColumnIndexByName("Percent")) {
    return QString("%1%").arg(
        this->labels->getRowColumnData(index.row(), "Percent").toFloat(), 0,
        'f', 1);
  }

  return QVariant();
}

QVariant LabelViewModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role != Qt::DisplayRole) return QVariant();

  if (orientation == Qt::Horizontal) {
    if (section == labels->getColumnIndexByName("LabelID")) {
      return "ID";
    } else if (section == labels->getColumnIndexByName("Export")) {
      return "Export";
    } else if (section == labels->getColumnIndexByName("Name")) {
      return "Name";
    } else if (section == labels->getColumnIndexByName("Visibility")) {
      return "Visible";
    } else if (section == labels->getColumnIndexByName("Voxels")) {
      return "Voxels";
    } else if (section == labels->getColumnIndexByName("Percent")) {
      return "Percent";
    } else {
      return "#<error>";
    }
  } else {
    return QVariant();
  }
}

Qt::ItemFlags LabelViewModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags flags =
      Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;

  // Set flags to make name editable and visibility checkable
  if (index.column() == labels->getColumnIndexByName("Name")) {
    flags |= Qt::ItemFlag::ItemIsEditable;
  } else if (index.column() == labels->getColumnIndexByName("Visibility")) {
    flags |= Qt::ItemFlag::ItemIsUserCheckable;
  } else if (index.column() == labels->getColumnIndexByName("Export")) {
    flags |= Qt::ItemFlag::ItemIsUserCheckable;
  }

  return flags;
}

bool LabelViewModel::setData(const QModelIndex& index, const QVariant& value,
                             int role) {
  // Update label name
  if (index.isValid() && role == Qt::EditRole &&
      index.column() == labels->getColumnIndexByName("Name")) {
    TableRow row =
        this->labels->getRowsByIndex(index.row(), index.row() + 1).first();
    if (this->stepManager)
      this->stepManager->createMetaStep(
          row.data().at(labels->getColumnIndexByName("LabelID")).toInt(),
          "Name", value);
    Q_EMIT dataChanged(index, index, {role});
    return true;
  }

  // Update visibility
  if (index.isValid() && role == Qt::CheckStateRole &&
      index.column() == labels->getColumnIndexByName("Visibility")) {
    TableRow row =
        this->labels->getRowsByIndex(index.row(), index.row() + 1).first();
    if (this->stepManager)
      this->stepManager->createMetaStep(
          row.data().at(labels->getColumnIndexByName("LabelID")).toInt(),
          "Visibility", value == Qt::CheckState::Checked);
    Q_EMIT dataChanged(index, index, {role});
    return true;
  }

  // Update Export
  if (index.isValid() && role == Qt::CheckStateRole &&
      index.column() == labels->getColumnIndexByName("Export")) {
    TableRow row =
        this->labels->getRowsByIndex(index.row(), index.row() + 1).first();

    // this change is only relevant for exporting --> we do not need to save it
    this->labels->modifyRowEntry(
        QSharedPointer<DataUpdate>(), row.rowID(),
        this->labels->getColumnIndexByName("Export"),
        value == Qt::CheckState::Checked ? true : false);

    Q_EMIT dataChanged(index, index, {role});
    return true;
  }

  return false;
}

bool LabelViewModel::insertRows(int row, int count, const QModelIndex& parent) {
  beginInsertRows(parent, row, row + count - 1);
  endInsertRows();
  return true;
}

bool LabelViewModel::removeRows(int row, int count, const QModelIndex& parent) {
  beginRemoveRows(parent, row, row + count - 1);
  endRemoveRows();
  return true;
}

int LabelViewModel::getLabelIDbyRowIdx(int rowIdx) {
  return this->labels->getRowsByIndex(rowIdx, rowIdx + 1)
      .first()
      .data()
      .at(labels->getColumnIndexByName("LabelID"))
      .toInt();
}

}  // namespace vx
