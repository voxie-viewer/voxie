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

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/SurfaceBuilder.hpp>
#include <Voxie/Data/TableNode.hpp>

#include <Voxie/Node/Types.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

#include <PluginFilter/Prototypes.hpp>

#include <QApplication>
#include <QInputDialog>
#include <QLabel>
#include <QObject>
#include <QSharedPointer>
#include <Voxie/Data/SurfaceNode.hpp>
#include "ColorizeLabeledSurface.hpp"
#include "ColorizeLabeledSurfaceOperation.hpp"

#include <VoxieBackend/IO/SharpThread.hpp>

using namespace vx::filters;
using namespace vx;
using namespace vx::io;

ColorizeLabeledSurface::ColorizeLabeledSurface()
    : FilterNode(getPrototypeSingleton()),
      properties(new ColorizeLabeledSurfaceProperties(this)),
      histogramProvider(decltype(histogramProvider)::create()) {
  qRegisterMetaType<QSharedPointer<SurfaceDataTriangleIndexed>>();
  qRegisterMetaType<QSharedPointer<RunFilterOperation>>();
  PropertySection* section = new PropertySection();
  this->setAutomaticDisplayName("Colorize Labeled Surface");

  columnKeySelector = new QComboBox();
  columnKeySelector->addItems({"-"});
  section->addProperty(columnKeySelector);

  columnValueSelector = new QComboBox();
  columnValueSelector->addItems({"Random"});
  section->addProperty(columnValueSelector);

  this->addPropertySection(section);

  connect(properties, &ColorizeLabeledSurfaceProperties::inputTableChanged,
          this, [this](Node* newParent) {
            setTable(dynamic_cast<TableNode*>(newParent));
          });

  connect(
      columnValueSelector,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &ColorizeLabeledSurface::updateSelectedColumn);
}

void ColorizeLabeledSurface::setTable(TableNode* table) {
  if (this->tableNode != nullptr) {
    disconnect(this->tableNode, &DataNode::dataChangedFinished, this,
               &ColorizeLabeledSurface::updateTable);
  }

  this->tableNode = table;

  if (table != nullptr) {
    connect(table, &DataNode::dataChangedFinished, this,
            &ColorizeLabeledSurface::updateTable);

    updateTable();
  }
}

QSharedPointer<TableData> ColorizeLabeledSurface::getTableData() const {
  TableNode* inputTable =
      dynamic_cast<TableNode*>(this->properties->inputTable());
  return inputTable ? inputTable->tableData() : QSharedPointer<TableData>();
}

void ColorizeLabeledSurface::updateTable() {
  columnKeySelector->clear();

  columnValueSelector->clear();
  columnValueSelector->addItem("Random", -1);

  if (auto table = getTableData()) {
    for (int i = 0; i < table->columns().size(); i++) {
      if (vx::types::IntType()->name() == table->columns()[i].type()->name() ||
          vx::types::FloatType()->name() ==
              table->columns()[i].type()->name()) {
        columnKeySelector->addItem(table->columns()[i].displayName(), i);
        columnValueSelector->addItem(table->columns()[i].displayName(), i);
      }
    }
  }

  if (columnKeySelector->count() <= 0) {
    columnKeySelector->addItem("-", -1);
  }

  updateSelectedColumn();
}

void ColorizeLabeledSurface::updateSelectedColumn() {
  int columnIndex = columnValueSelector->currentData().toInt();
  auto tableData = getTableData();
  if (tableData && columnIndex >= 0 &&
      columnIndex < tableData->columns().size()) {
    auto rows = tableData->getRowsByIndex();
    histogramProvider->setDataFromContainer(
        HistogramProvider::DefaultBucketCount, rows, [=](const TableRow& row) {
          return row.data()[columnIndex].toDouble();
        });
  }
}

QSharedPointer<QObject> ColorizeLabeledSurface::getPropertyUIData(
    QString propertyName) {
  if (propertyName ==
      "de.uni_stuttgart.Voxie.Filter.ColorizeLabeledSurface.InputColorizer") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

QSharedPointer<RunFilterOperation> ColorizeLabeledSurface::calculate() {
  QSharedPointer<RunFilterOperation> operation(
      new RunFilterOperation(), [](QObject* obj) { obj->deleteLater(); });

  // This object will be moved to the newly created thread and will be deleted
  // on this thread once the operation is finished
  auto colorizeOperation = new ColorizeLabeledSurfaceOperation(operation);

  QList<vx::ColorizerEntry> colorizerEntries =
      this->properties->inputColorizer();

  Colorizer* colorizer = new Colorizer();
  colorizer->setEntries(colorizerEntries);

  SurfaceNode* inputSurface =
      dynamic_cast<SurfaceNode*>(this->properties->inputSurface());
  if (!inputSurface) {
    qWarning() << "ColorizeLabeledSurface::calculate(): Could not find a "
                  "SurfaceNode parent node";
    return operation;
  }

  TableNode* inputTable =
      dynamic_cast<TableNode*>(this->properties->inputTable());
  int tableColumnKeyIndex = -1;
  int tableColumnValueIndex = -1;
  QSharedPointer<TableData> tableData;
  if (inputTable) {
    tableData = inputTable->tableData();
  }

  if (!tableData) {
    qWarning() << "ColorizeLabeledSurface::calculate(): Could not find a "
                  "TableData parent node";
  } else {
    // selected column key index
    tableColumnKeyIndex = columnKeySelector->currentData().toInt();
    // selected column value index
    tableColumnValueIndex = columnValueSelector->currentData().toInt();
  }

  SharpThread* thread = new SharpThread([=]() -> void {
    colorizeOperation->colorizeModel(inputSurface, colorizer, tableData,
                                     tableColumnKeyIndex,
                                     tableColumnValueIndex);
    delete colorizeOperation;
    delete colorizer;
  });
  colorizeOperation->moveToThread(thread);

  // When the isosurface gets destroyed, cancel the operation
  // The thread might continue in the background until it actually processes
  // the cancellation, but this result (if any) will be ignored
  connect(this, &QObject::destroyed, operation.data(), &Operation::cancel);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  connect(colorizeOperation, &ColorizeLabeledSurfaceOperation::colorizationDone,
          this, &ColorizeLabeledSurface::updateSurface);

  OperationRegistry::instance()->addOperation(operation);

  thread->start();
  return operation;
}

void ColorizeLabeledSurface::updateSurface(
    const QSharedPointer<SurfaceDataTriangleIndexed>& outputSurface,
    QSharedPointer<RunFilterOperation> operation) {
  if (outputSurface) {
    QSharedPointer<SurfaceNode> surfaceNode;

    for (Node* childNode : this->childNodes()) {
      if (SurfaceNode* output = dynamic_cast<SurfaceNode*>(childNode)) {
        surfaceNode = output->thisShared();
      }
    }

    if (!surfaceNode) {
      surfaceNode = createNode<SurfaceNode>();
      this->addChildNode(surfaceNode.data());
    }

    surfaceNode->setSurface(outputSurface);
  }
  operation->emitFinished();
}

NODE_PROTOTYPE_IMPL(ColorizeLabeledSurface)
