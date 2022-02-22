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

#include <Voxie/Data/TableNode.hpp>

#include <PluginVisTable/Prototypes.forward.hpp>

#include <Voxie/Vis/VisualizerNode.hpp>
#include <Voxie/Vis/VisualizerView.hpp>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>

#include <QtGui/QPaintEvent>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

class ImageSelectionWidget;

namespace vx {
class TableViewWidget;

/**
 * @brief TableVisualizer displays TableData nodes as a text-based table.
 */
class TableVisualizer : public vx::visualization::SimpleVisualizer {
  Q_OBJECT

 private:
  vx::TableNode* tableNode = nullptr;

  QWidget* viewRoot = nullptr;
  vx::TableViewWidget* tableView = nullptr;

  QComboBox* columnSelector = nullptr;

  QTimer* tableUpdateTimer = nullptr;

  int sortColumn = 0;
  Qt::SortOrder sortOrder = Qt::AscendingOrder;

 public:
  TableVisualizer();
  virtual ~TableVisualizer() override;

  void setTable(vx::TableNode* table);
  vx::TableNode* table() const;
  QSharedPointer<vx::TableData> tableData() const;

  vx::SharedFunPtr<RenderFunction> getRenderFunction() override;

  QVariant getNodePropertyCustom(QString key) override;
  void setNodePropertyCustom(QString key, QVariant value) override;

  QWidget* mainView() override;

  NODE_PROTOTYPE_DECL_SEP(visualizer_prop::Table, TableVisualizer)

 protected:
  QWidget* getCustomPropertySectionContent(const QString& name) override;

 private:
  void updateTable();
  QString lookUpColumnName(int index) const;
  QString lookUpSortOrder(Qt::SortOrder order) const;

  void setTableSortColumn(QString columnName);
  void setTableSortOrder(QString sortOrderEnum);
};
}  // namespace vx
