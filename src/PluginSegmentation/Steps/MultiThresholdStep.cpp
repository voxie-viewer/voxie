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

#include "MultiThresholdStep.hpp"

#include <PluginSegmentation/Steps/MetaStep.hpp>

#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <Voxie/Data/VolumeNode.hpp>

using namespace vx;
using namespace vx::io;

MultiThresholdStep::MultiThresholdStep()
    : SegmentationStep("MultiThresholdStep", getPrototypeSingleton()),
      properties(new MultiThresholdStepProperties(this)) {}

MultiThresholdStep::~MultiThresholdStep() {}

QSharedPointer<OperationResult> MultiThresholdStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  return this->runThreaded("MultiThresholdStep", [parameterCopy, containerData,
                                                  inputVolume,
                                                  this](const QSharedPointer<
                                                        Operation>& op) {
    MultiThresholdStepPropertiesCopy propertyCopy(
        parameterCopy->properties()[parameterCopy->mainNodePath()]);

    auto labelTable = qSharedPointerDynamicCast<TableData>(
        containerData->getElement("labelTable"));

    // cast output to VolumeDataVoxelInst SegmentationType as we know its type
    auto labelVolume =
        qSharedPointerDynamicCast<VolumeDataVoxelInst<SegmentationType>>(
            containerData->getElement("labelVolume"));

    auto originalVolume = QSharedPointer<VolumeDataVoxel>();

    if (propertyCopy.volume()) {
      originalVolume = qSharedPointerDynamicCast<VolumeDataVoxel>(
          dynamic_cast<VolumeNode*>(propertyCopy.volume())->data());
    }  // no volume in properties --> take inputVolume of Segmentation
    else {
      originalVolume = qSharedPointerDynamicCast<VolumeDataVoxel>(
          dynamic_cast<VolumeNode*>(
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
                  inputVolume))
              ->data());
    }

    const vx::VectorSizeT3& inputDimensions = originalVolume->getDimensions();

    auto outerUpdate = containerData->createUpdate();

    auto thresholdList = propertyCopy.thresholdList();

    // get existing label ids
    QList<SegmentationType> existingLabelIDs;
    for (quint64 rowIndex = 0; rowIndex < labelTable->rowCount(); rowIndex++) {
      existingLabelIDs.append(
          labelTable->getRowColumnData(rowIndex, "LabelID").toUInt());
    }

    for (auto entry : thresholdList) {
      qint64 labelId = std::get<2>(entry);
      // create labels. -2 is a threshold entry for + inifinity
      if (!existingLabelIDs.contains(labelId) && labelId != -2) {
        auto tableUpdate =
            labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

        labelTable->addRow(
            tableUpdate,
            {false, (qlonglong)labelId, QString("Label #%1").arg(labelId), true,
             (qlonglong)0, 0.0, QVariant::fromValue(std::get<1>(entry)), ""});

        tableUpdate->finish({});

        // Update the StepWidget, if the step is not in the stepList
        if (!this->multiThreshold.isNull()) {
          auto colorizerLabelMap = this->multiThreshold->getColorizerLabelMap();
          colorizerLabelMap[thresholdList.indexOf(entry)] = labelId;
          this->multiThreshold->setColorizerLabelMap(colorizerLabelMap);
        }
      }
    }

    auto update =
        labelVolume->createUpdate({{containerData->getPath(), outerUpdate}});

    if (thresholdList.isEmpty()) {
      update->finish({});
      outerUpdate->finish({});
      return;
    }

    // cast VolumeVoxelData to VolumeVoxelDataInst inside lambda function
    originalVolume->performInGenericContext([=](auto& originalVolumeInst) {
      QMap<qint64, qint64> labelVoxelChangeMap =
          initLabelVoxelChangeMap(labelTable);
      for (size_t x = 0; x < inputDimensions.x; x++) {
        for (size_t y = 0; y < inputDimensions.y; y++) {
          op->throwIfCancelled();
          for (size_t z = 0; z < inputDimensions.z; z++) {
            auto voxelVal = originalVolumeInst.getVoxel(x, y, z);
            QList<std::tuple<double, std::tuple<double, double, double, double>,
                             qint64>>::const_iterator i;

            for (i = thresholdList.begin(); i != --thresholdList.end(); ++i) {
              // check if the input voxel is >= the lower threshold and < the
              // upper threshold
              if (voxelVal >= std::get<0>(*i) &&
                  voxelVal < std::get<0>(*(i + 1))) {
                // get the label ID assigned to this threshold range
                int labelId = std::get<2>(*i);
                if (labelId >= 0) {
                  labelVolume->setVoxel(x, y, z, labelId);
                } else {
                  labelId = labelVolume->getVoxel(x, y, z);
                }
                // create the labelVoxelChangeMap by tracking all value counts
                labelVoxelChangeMap[labelId]++;
              }
            }
          }
        }
        op->updateProgress(1.0f * x / inputDimensions.x);
      }
      update->finish({});
      outerUpdate->finish({});
      updateStatistics(containerData, labelVoxelChangeMap, false);
      Q_EMIT(this->updateSelectedVoxelCount(0, false));
    });
  });
}

QString MultiThresholdStep::getInfoString() {
  QString info = "MultiThresholdStep Threshold to Labels:[";
  auto threshList = this->properties->thresholdList();
  QList<std::tuple<double, std::tuple<double, double, double, double>,
                   qint64>>::const_iterator i;
  for (i = threshList.begin(); i != --threshList.end(); ++i) {
    info.append(QString::number(std::get<0>(*i)));
    info.append("->");
    info.append(QString::number(std::get<0>(*(i + 1))));
    info.append(" to ID ");
    info.append(QString::number(std::get<2>(*i)));
    if (i != (--threshList.end())) {
      info.append(", ");
    }
  }
  info.append("]");
  return info;
}

void MultiThresholdStep::resetUIWidget() {}

void MultiThresholdStep::onVoxelOpFinished(bool status) { Q_UNUSED(status); }

QWidget* MultiThresholdStep::getCustomPropertySectionContent(
    const QString& name) {
  this->initializeCustomUIPropSections();
  if (name ==
      "de.uni_stuttgart.Voxie.SegmentationStep.MultiThresholdStep."
      "MultiThresWidget") {
    return this->multiThreshold;
  } else {
    return Node::getCustomPropertySectionContent(name);
  }
}

bool MultiThresholdStep::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

bool MultiThresholdStep::isAllowedParent(NodeKind node) {
  return node == NodeKind::Data;
}
bool MultiThresholdStep::isCreatableChild(NodeKind) { return false; }

QList<QString> MultiThresholdStep::supportedDBusInterfaces() { return {}; }

void MultiThresholdStep::initializeCustomUIPropSections() {
  this->multiThreshold = new ThresholdWidget(this->labelViewModel);
  multiThreshold->init();

  this->multiThreshold->getColorizerWidget()->setHistogramProvider(
      this->histogramProvider);

  connect(this, &SegmentationStep::histogramProviderChanged, this,
          [=](QSharedPointer<vx::HistogramProvider> histProvider) {
            this->multiThreshold->getColorizerWidget()->setHistogramProvider(
                histProvider);
          });

  connect(this->labelViewModel->getLabelTable().data(), &vx::Data::dataChanged,
          this, [=]() { this->multiThreshold->updateLabelList(true, true); });

  connect(this->multiThreshold, &ThresholdWidget::dataChanged, this, [=]() {
    // Add colors that are chosen in the MultiThresWidget as Labels
    QList<
        std::tuple<double, std::tuple<double, double, double, double>, qint64>>
        multiThreshList;
    QMap<int, int>::iterator l;
    auto colorizerLabelMap = this->multiThreshold->getColorizerLabelMap();

    int labelIdIdxCounter = 0;
    QList<int> labelIDCounter =
        this->labelViewModel->getLabelIdCounter(colorizerLabelMap.size());

    for (l = colorizerLabelMap.begin(); l != colorizerLabelMap.end(); ++l) {
      vx::ColorizerEntry entry =
          this->multiThreshold->getColorizerWidget()->getEntry(l.key());
      int labelId = l.value();

      if (labelId == -1) {
        // Assign labelIDs to missing labels
        labelId = labelIDCounter[labelIdIdxCounter];

        labelIdIdxCounter++;
      }

      multiThreshList.append(std::make_tuple(
          entry.value(), entry.color().asTuple(), (qint64)labelId));
    }

    this->properties->setThresholdList(multiThreshList);
  });
}

NODE_PROTOTYPE_IMPL(MultiThresholdStep)
