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
#include <QPushButton>
#include <QtDBus/QDBusObjectPath>
#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/ContainerNode.hpp>
#include <Voxie/Data/LabelViewModel.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/ParameterCopy.hpp>
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>
#include <VoxieClient/QtUtil.hpp>

namespace vx {

enum class StepKind : uint32_t {
  MetaStep,
  LabelStep,
  SelectionStep,
  ExtensionStep,
  None
};

class VOXIECORESHARED_EXPORT SegmentationStep : public Node {
  Q_OBJECT

 protected:
  /**
   * @brief Each step holds a reference to the labelViewModel, in case the
   * UI-Widgets of the step need information about the labels
   */
  LabelViewModel* labelViewModel;
  /**
   * @brief Each step holds a reference to the current histogramProvider of the
   * inputVolume, in case the UI-Widgets of the step need the histogram
   */
  QSharedPointer<vx::HistogramProvider> histogramProvider;

  static QSharedPointer<OperationResult> runThreaded(
      const QString& description,
      std::function<void(const QSharedPointer<vx::io::Operation>&)> f);

 private:
  PropertySection* propSection;

  QPushButton* runStepButton;
  QSharedPointer<PropertyCondition> runStepEnabledCondition_;

  /**
   * @brief Adds the custom UI elements to the propSection Widget
   */
  virtual void initializeCustomUIPropSections() = 0;

  /**
   * @brief Functions related to the runStep button. The runStep Button does not
   * need to be used
   */
  void initializeRunStepButton();
  void initializeRunStepEnabledCondition();
  void updateRunStepButtonState();

 Q_SIGNALS:
  void updateSelectedVoxelCount(qint64 voxelCount, bool incremental);
  void histogramProviderChanged(
      QSharedPointer<vx::HistogramProvider> histProvider);
  void triggerCalculation();

 public:
  SegmentationStep(QString prototypeName,
                   const QSharedPointer<NodePrototype>& prototype);
  SegmentationStep();
  virtual ~SegmentationStep();

  /**
   * @brief returns a clone of the current step. The clone will have the same
   * properties than the original step
   * @param registerNode boolean to indicate, if the step should be registered
   * in the voxie graph
   */
  QSharedPointer<SegmentationStep> clone(bool registerNode = true);

  /**
   * @brief Should contain all code that is related to the direct execution of
   * the step functionalities
   * @param containerData labelContainer that contains the labelVolume and the
   * labelTable
   * @param inputVolume input volume data DBus path of the Segmentation Filter
   */
  virtual QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) = 0;

  /**
   * @brief returns an info string that should contain the step name and
   * important information regarding the step
   */
  virtual QString getInfoString() = 0;

  /**
   * @brief Resets the UI-Widget to the default state
   */
  virtual void resetUIWidget() = 0;

  /**
   * @brief Function that should be called, when the stepManager emmits
   * VoxelOpFinished (Used to block functionality while other operations are
   * executed)
   * @param status status of voxel operation finished state, false=not finished,
   * true=finished
   */
  virtual void onVoxelOpFinished(bool status) = 0;

  /**
   * @brief Adds the property section UI elements (Default or Custom) to the
   * propertySection
   */
  void fillPropertySection(LabelViewModel* labelViewModel);

  void setPropertySection(PropertySection* section) {
    this->propSection = section;
  }

  QSharedPointer<PropertyCondition> getRunStepEnabledCondition() const {
    return this->runStepEnabledCondition_;
  }

  LabelViewModel* getLabelViewModel() { return this->labelViewModel; }

  void setLabelViewModel(LabelViewModel* labelViewModel) {
    this->labelViewModel = labelViewModel;
  }
  void setHistogramProvider(QSharedPointer<HistogramProvider> histProvider) {
    this->histogramProvider = histProvider;
    Q_EMIT this->histogramProviderChanged(histProvider);
  }

  QPushButton* getRunStepButton() { return this->runStepButton; }

  PropertySection* getPropertySection() { return this->propSection; }

  QString getStepTip() {
    return this->prototype()->rawJson()["StepTip"].toString();
  }

  StepKind getStepKind();

  // Get information about filename and last modification of of the extension,
  // if any, otherwise an empty map.
  virtual QMap<QString, QString> getExtensionInfo();
};

}  // namespace vx
