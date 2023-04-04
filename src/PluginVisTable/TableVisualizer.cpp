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
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include <PluginVisTable/Prototypes.hpp>
#include <PluginVisTable/TableUtils.hpp>
#include <PluginVisTable/TableViewWidget.hpp>
#include <PluginVisTable/TableVisualizer.hpp>

#include <Voxie/Data/TableColumnListView.hpp>
#include <Voxie/Data/TableData.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyUI.hpp>

#include <Voxie/IVoxie.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMetaType>

#include <QtGui/QColor>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QRgb>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>

using namespace vx;
using namespace vx::visualization;

static QWidget* labeledWidget(QString labelText, QWidget* widget) {
  QWidget* labeledWidget = new QWidget;
  labeledWidget->setLayout(new QHBoxLayout);
  labeledWidget->layout()->setContentsMargins(0, 0, 0, 0);

  QLabel* label = new QLabel(labelText + ":");
  labeledWidget->layout()->addWidget(label);
  labeledWidget->layout()->addWidget(widget);

  return labeledWidget;
}

TableVisualizer::TableVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()),
      properties(new visualizer_prop::TableProperties(this)) {
  this->viewRoot = new QWidget;
  this->viewRoot->setMinimumSize(300, 200);

  QGridLayout* layout = new QGridLayout;
  this->viewRoot->setLayout(layout);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  this->tableView = new TableViewWidget;
  layout->addWidget(this->tableView, 0, 0);

  QWidget* sortSection = new QWidget;
  sortSection->setLayout(new QVBoxLayout());
  sortSection->layout()->setContentsMargins(0, 0, 0, 0);
  sortSection->setWindowTitle(tr("Sorting"));
  addPropertySection(sortSection);

  this->tableUpdateTimer = new QTimer(this);
  this->tableUpdateTimer->setSingleShot(true);
  this->tableUpdateTimer->setTimerType(Qt::TimerType::PreciseTimer);
  this->tableUpdateTimer->setInterval(500);

  sortSection->layout()->addWidget(
      labeledWidget(properties->sortOrderProperty()->displayName(),
                    vx::types::EnumerationType()
                        ->createUI(properties->sortOrderProperty(), this)
                        ->widget()));

  columnSelector = new QComboBox;
  sortSection->layout()->addWidget(labeledWidget(
      properties->sortColumnProperty()->displayName(), columnSelector));

  connect(properties, &visualizer_prop::TableProperties::rowLimitChanged, this,
          [this](qint64 rowLimit) {
            tableView->setRowLimit(static_cast<std::size_t>(rowLimit));
            tableUpdateTimer->start();
          });

  connect(tableUpdateTimer, &QTimer::timeout, this,
          &TableVisualizer::updateTable);

  connect(
      properties, &visualizer_prop::TableProperties::sortColumnChanged, this,
      [this](QString sortColumn) {
        columnSelector->setCurrentIndex(columnSelector->findData(sortColumn));
      });

  connect(
      columnSelector,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, [this](int index) {
        properties->setSortColumn(columnSelector->itemData(index).toString());
      });

  connect(properties, &visualizer_prop::TableProperties::tableChanged, this,
          [this](Node* newParent) {
            setTable(dynamic_cast<TableNode*>(newParent));
          });

  updateTable();
}

TableVisualizer::~TableVisualizer() {}

void TableVisualizer::setTable(TableNode* table) {
  if (this->tableNode != nullptr) {
    disconnect(this->tableNode, &DataNode::dataChangedFinished, this,
               &TableVisualizer::updateTable);
  }

  this->tableNode = table;

  if (table != nullptr) {
    connect(table, &DataNode::dataChangedFinished, this,
            &TableVisualizer::updateTable);

    updateTable();
  }
}

TableNode* TableVisualizer::table() const { return this->tableNode; }

QSharedPointer<TableData> TableVisualizer::tableData() const {
  return table() != nullptr ? table()->tableData()
                            : QSharedPointer<TableData>();
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
TableVisualizer::getRenderFunction() {
  return [](const QSharedPointer<ImageDataPixel>& outputImage,
            const VectorSizeT2& outputRegionStart, const VectorSizeT2& size,
            const QSharedPointer<ParameterCopy>& parameters,
            const QSharedPointer<VisualizerRenderOptions>& options) {
    // TODO: implement render function
    Q_UNUSED(parameters);
    Q_UNUSED(options);

    QImage image(static_cast<int>(size.x), static_cast<int>(size.y),
                 QImage::Format_ARGB32);
    image.fill(qRgba(0, 0, 0, 0));

    outputImage->fromQImage(image, outputRegionStart);
  };
}

QWidget* TableVisualizer::mainView() { return this->viewRoot; }

QWidget* TableVisualizer::getCustomPropertySectionContent(const QString& name) {
  Q_UNUSED(name);
  return new QWidget;
}

QVariant TableVisualizer::getNodePropertyCustom(QString key) {
  if (key == "de.uni_stuttgart.Voxie.Visualizer.Table.SortColumn") {
    return lookUpColumnName(sortColumn);
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Table.SortOrder") {
    return lookUpSortOrder(sortOrder);
  } else {
    return VisualizerNode::getNodePropertyCustom(key);
  }
}

void TableVisualizer::setNodePropertyCustom(QString key, QVariant value) {
  if (key == "de.uni_stuttgart.Voxie.Visualizer.Table.SortColumn") {
    setTableSortColumn(value.toString());
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Table.SortOrder") {
    setTableSortOrder(value.toString());
  } else {
    VisualizerNode::setNodePropertyCustom(key, value);
  }
}

void TableVisualizer::updateTable() {
  QString title = "Table visualizer";

  if (table() != nullptr) {
    // Update inner window title
    title += " - " + table()->displayName();

    // Save the current sorting column
    auto currentSortColumn = properties->sortColumn();

    // Populate the sorting column selection combobox
    columnSelector->clear();
    if (auto tableData = table()->tableData()) {
      for (auto& column : tableData->columns()) {
        if (TableUtils::isComparableType(column.type())) {
          columnSelector->addItem(column.displayName(), column.name());
        }
      }
    }

    // Update the table
    tableView->updateTable(table()->tableData());

    // Restore the sorting column
    properties->setSortColumn(currentSortColumn);
  }

  setAutomaticDisplayName(title);
}

QString TableVisualizer::lookUpColumnName(int index) const {
  if (tableData() != nullptr) {
    const auto& columns = tableData()->columns();
    return index >= 0 && index < columns.size() ? columns[index].name() : "";
  } else {
    return "";
  }
}

QString TableVisualizer::lookUpSortOrder(Qt::SortOrder order) const {
  return order == Qt::AscendingOrder
             ? "de.uni_stuttgart.Voxie.Visualizer.Table.SortOrder.Ascending"
             : "de.uni_stuttgart.Voxie.Visualizer.Table.SortOrder.Descending";
}

void TableVisualizer::setTableSortColumn(QString columnName) {
  if (tableData() != nullptr && !tableData()->columns().empty()) {
    auto& header = *tableView->horizontalHeader();

    // Disambiguate sort order by first column
    // TODO: use a custom table model with a sort proxy instead
    // TODO: This will print a lot of warnings if the first column is not
    // sortable
    header.setSortIndicator(0, Qt::AscendingOrder);

    const auto& columns = tableData()->columns();
    for (int i = 0; i < columns.size(); ++i) {
      if (columns[i].name() == columnName) {
        // Column with matching name was found: sort by this column
        header.setSortIndicator(i, sortOrder);

        // Store sorting column index
        sortColumn = i;
        return;
      }
    }

    // No column with a matching name was found: use natural sorting
    header.setSortIndicator(-1, sortOrder);
  }
}

void TableVisualizer::setTableSortOrder(QString sortOrderEnum) {
  auto& header = *tableView->horizontalHeader();

  header.setSortIndicator(
      header.sortIndicatorSection(),
      sortOrderEnum ==
              "de.uni_stuttgart.Voxie.Visualizer.Table.SortOrder.Descending"
          ? Qt::DescendingOrder
          : Qt::AscendingOrder);

  // Store sorting order
  sortOrder = header.sortIndicatorOrder();
}

NODE_PROTOTYPE_IMPL_SEP(visualizer_prop::Table, TableVisualizer)
