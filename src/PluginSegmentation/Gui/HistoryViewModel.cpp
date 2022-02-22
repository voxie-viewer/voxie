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

#include <PluginSegmentation/Gui/HistoryViewModel.hpp>
#include <Voxie/Node/SegmentationStep.hpp>

namespace vx {

HistoryViewModel::HistoryViewModel(
    SegmentationProperties* segmentationProperties)
    : segmentationProperties(segmentationProperties) {
  connect(segmentationProperties, &SegmentationProperties::stepListChanged,
          this, [this](QList<vx::Node*> value) {
            Q_UNUSED(value);
            this->updateView(1);
          });
}

int HistoryViewModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent);
  return this->segmentationProperties->stepList().size();
}

QVariant HistoryViewModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  if (index.row() >= this->segmentationProperties->stepList().size())
    return QVariant();

  // Return info string of the SegmentationStep
  if (role == Qt::DisplayRole && index.column() == 0) {
    SegmentationStep* step =
        (SegmentationStep*)segmentationProperties->stepList().at(index.row());
    return step->getInfoString();
  }

  return QVariant();
}

bool HistoryViewModel::insertRows(int row, int count,
                                  const QModelIndex& parent) {
  beginInsertRows(parent, row, row + count - 1);
  endInsertRows();
  return true;
}

bool HistoryViewModel::removeRows(int row, int count,
                                  const QModelIndex& parent) {
  beginRemoveRows(parent, row, row + count - 1);
  endRemoveRows();
  return true;
}

void HistoryViewModel::updateViewSlot(const QModelIndex& topLeft,
                                      const QModelIndex& bottomRight,
                                      const QVector<int>& roles) {
  Q_UNUSED(roles);
  updateView(bottomRight.row() - topLeft.row() + 1);
}

void HistoryViewModel::updateView(int newRowCount) {
  insertRows(rowCount() - newRowCount, newRowCount);
}

}  // namespace vx
