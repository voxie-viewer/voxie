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

#include "CcaResultSelection.hpp"

#include <Voxie/Data/LabelAttributes.hpp>
#include <Voxie/Data/LabelConstraint.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/Data/TableNode.hpp>

#include <PluginFilter/Prototypes.hpp>

#include <QHBoxLayout>
#include <QLabel>
#include <QMutableListIterator>
#include <QVBoxLayout>
#include <QWidget>

VX_NODE_INSTANTIATION(vx::filters::TableFilter)

using namespace vx;
using namespace vx::filters;

TableFilter::TableFilter()
    : FilterNode(getPrototypeSingleton()),
      properties(new TableFilterProperties(this)) {
  idCounter = 0;
  this->setAutomaticDisplayName("CCA Result Selection");
  this->selectProperties = new PropertySection();
  this->addPropertySection(selectProperties);

  // TODO: Clean this up, the output node should not be created here (and the
  // reference should not be stored here)
  this->selectedCcaResults = createNode<TableNode>().data();
  selectedCcaResults->setAutomaticDisplayName("Selected CCA Results");

  QPushButton* newConstraint = new QPushButton();
  newConstraint->setIcon(QIcon(":/icons/plus.png"));
  newConstraint->setText("Create new Constraint");
  connect(newConstraint, &QPushButton::released, this,
          &TableFilter::createConstraint);
  this->selectProperties->addProperty(newConstraint);
  this->createConstraint();
}

void TableFilter::createConstraint() {
  LabelConstraint* constraint = new LabelConstraint(idCounter);
  this->constraints << constraint;
  connect(constraint, &LabelConstraint::constrainDeleted, this,
          &TableFilter::removeConstraint);
  idCounter += 1;
  this->selectProperties->addHBoxLayout(constraint);
}

QSharedPointer<vx::io::RunFilterOperation> TableFilter::calculate(
    bool isAutomaticFilterRun) {
  Q_UNUSED(isAutomaticFilterRun);
  QSharedPointer<vx::io::RunFilterOperation> operation =
      vx::io::RunFilterOperation::createRunFilterOperation();
  TableNode* labelObj = nullptr;
  for (Node* node : this->parentNodes()) {
    labelObj = dynamic_cast<TableNode*>(node);
    if (labelObj) break;
  }
  if (!labelObj) {
    qWarning() << "No table node";
    return operation;
  }

  auto srcData = labelObj->tableData();
  if (!srcData) {
    this->selectedCcaResults->setData(srcData);
    return operation;
  }
  auto resData = TableData::create(srcData->columns());
  auto update = resData->createUpdate();

  for (const auto& row : srcData->getRowsByIndex()) {
    for (LabelConstraint* constraint : constraints) {
      double currentValue = LabelAttributes::getValues(
          srcData->columns(), row, constraint->attributeBox->currentText());
      QString op = constraint->operatorBox->currentText();
      double limit = constraint->value;
      if (this->checkConstraint(currentValue, op, limit)) {
        resData->addRow(update, row.data());
      }
    }
  }

  update->finish(QJsonObject());

  this->selectedCcaResults->setData(resData);
  // TODO
  if (this->childNodes().length() == 0) {
    this->addChildNode(selectedCcaResults);
  }
  return operation;
}

void TableFilter::removeConstraint(int id) {
  QMutableListIterator<LabelConstraint*> iterator(this->constraints);
  while (iterator.hasNext()) {
    if (iterator.next()->getID() == id) {
      iterator.remove();
    }
  }
}

bool TableFilter::checkConstraint(int currentValue, QString op, double limit) {
  if (op == "=") {
    return (currentValue == limit);
  }
  if (op == "<") {
    return (currentValue < limit);
  }
  if (op == ">") {
    return (currentValue > limit);
  }
  if (op == "<=") {
    return (currentValue <= limit);
  }
  if (op == ">=") {
    return (currentValue >= limit);
  }
  return true;
}

QVariant TableFilter::getNodePropertyCustom(QString key) {
  return FilterNode::getNodePropertyCustom(key);
}

void TableFilter::setNodePropertyCustom(QString key, QVariant value) {
  FilterNode::setNodePropertyCustom(key, value);
}
