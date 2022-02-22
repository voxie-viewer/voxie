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

#include "MetaStep.hpp"
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

using namespace vx;
using namespace vx::io;

MetaStep::MetaStep()
    : SegmentationStep("MetaStep", getPrototypeSingleton()),
      properties(new MetaStepProperties(this)) {}

MetaStep::~MetaStep() {}

QSharedPointer<OperationResult> MetaStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  Q_UNUSED(inputVolume);

  return this->runThreaded(
      "MetaStep", [parameterCopy, containerData,
                   this](const QSharedPointer<Operation>& op) {
        // MetaStep is fast enough that cancelation is not needed
        Q_UNUSED(op);

        MetaStepPropertiesCopy propertyCopy(
            parameterCopy->properties()[parameterCopy->mainNodePath()]);

        auto labelTable = qSharedPointerDynamicCast<TableData>(
            containerData->getElement("labelTable"));

        if (!(propertyCopy.labelID() == -1)) {
          qlonglong labelID = propertyCopy.labelID();

          if (propertyCopy.modificationKind() == "AddLabel") {
            addLabel(containerData, labelTable, labelID, propertyCopy.name(),
                     propertyCopy.color(), propertyCopy.description(), true);
          } else if (propertyCopy.modificationKind() == "Visibility") {
            changeVisibility(containerData, labelTable, labelID,
                             propertyCopy.visibility());
          } else if (propertyCopy.modificationKind() == "Color") {
            changeColor(containerData, labelTable, labelID,
                        propertyCopy.color());
          } else if (propertyCopy.modificationKind() == "Name") {
            changeName(containerData, labelTable, labelID, propertyCopy.name());
          } else if (propertyCopy.modificationKind() == "Description") {
            changeDescription(containerData, labelTable, labelID,
                              propertyCopy.description());
          } else {
            throw vx::Exception(
                "de.uni_stuttgart.Voxie.Error",
                QString("No appropriate modificationKind passed: %1")
                    .arg(propertyCopy.modificationKind()));
          }
        } else {
          throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                              QString("No LabelID provided in MetaStep"));
        }
      });
}

QString MetaStep::getInfoString() {
  if (properties->modificationKind() == "AddLabel") {
    return QString("Add label %1").arg(QString::number(properties->labelID()));
  }
  if (properties->modificationKind() == "Visibility") {
    if (properties->visibility()) {
      return QString("Show voxels of label %2")
          .arg(QString::number(properties->labelID()));
    } else {
      return QString("Hide voxels of label %2")
          .arg(QString::number(properties->labelID()));
    }
  }
  if (properties->modificationKind() == "Color") {
    return QString("Change color of label %1 to %2")
        .arg(QString::number(properties->labelID()))
        .arg(properties->color().asQColor().name());
  }
  if (properties->modificationKind() == "Name") {
    return QString("Change name of label %1 to '%2'")
        .arg(QString::number(properties->labelID()))
        .arg(properties->name());
  }
  if (properties->modificationKind() == "Description") {
    return QString("Change description of label %1")
        .arg(QString::number(properties->labelID()));
  }
  return QString("Unknown MetaStep");
}

void MetaStep::changeVisibility(QSharedPointer<ContainerData> containerData,
                                QSharedPointer<TableData> labelTable,
                                int labelID, bool visibility) {
  TableRow row = getTableRowByLabelID(labelID, labelTable);

  auto outerUpdate = containerData->createUpdate();
  auto update =
      labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

  labelTable->modifyRowEntry(update, row.rowID(),
                             labelTable->getColumnIndexByName("Visibility"),
                             visibility);

  update->finish({});
  outerUpdate->finish({});
}

void MetaStep::changeColor(QSharedPointer<ContainerData> containerData,
                           QSharedPointer<TableData> labelTable, int labelID,
                           Color newColor) {
  TableRow row = getTableRowByLabelID(labelID, labelTable);

  auto outerUpdate = containerData->createUpdate();
  auto update =
      labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

  labelTable->modifyRowEntry(update, row.rowID(),
                             labelTable->getColumnIndexByName("Color"),
                             QVariant::fromValue(newColor.asTuple()));

  update->finish({});
  outerUpdate->finish({});
}
void MetaStep::changeName(QSharedPointer<ContainerData> containerData,
                          QSharedPointer<TableData> labelTable, int labelID,
                          QString newName) {
  TableRow row = getTableRowByLabelID(labelID, labelTable);

  auto outerUpdate = containerData->createUpdate();
  auto update =
      labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

  labelTable->modifyRowEntry(update, row.rowID(),
                             labelTable->getColumnIndexByName("Name"), newName);

  update->finish({});
  outerUpdate->finish({});
}
void MetaStep::changeDescription(QSharedPointer<ContainerData> containerData,
                                 QSharedPointer<TableData> labelTable,
                                 int labelID, QString newDescription) {
  TableRow row = getTableRowByLabelID(labelID, labelTable);

  auto outerUpdate = containerData->createUpdate();
  auto update =
      labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

  labelTable->modifyRowEntry(update, row.rowID(),
                             labelTable->getColumnIndexByName("Description"),
                             newDescription);

  update->finish({});
  outerUpdate->finish({});
}

void MetaStep::addLabel(QSharedPointer<ContainerData> containerData,
                        QSharedPointer<TableData> labelTable, qlonglong labelID,
                        QString newName, Color newColor, QString newDescription,
                        bool visibility) {
  auto outerUpdate = containerData->createUpdate();
  auto update =
      labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

  labelTable->addRow(update,
                     {false, labelID, newName, visibility, (qlonglong)0, 0.0,
                      QVariant::fromValue(newColor.asTuple()), newDescription});

  update->finish({});
  outerUpdate->finish({});
}

void MetaStep::resetUIWidget() {}

void MetaStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }
bool MetaStep::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

bool MetaStep::isAllowedParent(NodeKind node) { return node == NodeKind::Data; }
bool MetaStep::isCreatableChild(NodeKind) { return false; }

QList<QString> MetaStep::supportedDBusInterfaces() { return {}; }

void MetaStep::initializeCustomUIPropSections() {}

NODE_PROTOTYPE_IMPL(MetaStep)
