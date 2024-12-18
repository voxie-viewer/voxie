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

#include <Main/Root.hpp>

#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/LabelViewModel.hpp>
#include <Voxie/Data/VolumeNode.hpp>

#include <Voxie/Interfaces/SliceVisualizerI.hpp>
#include <Voxie/Interfaces/StepManagerI.hpp>

#include <PluginSegmentation/Prototypes.hpp>
#include <PluginSegmentation/SegmentationUtils.hpp>
#include <PluginSegmentation/Steps/BrushSelectionStep.hpp>
#include <PluginSegmentation/Steps/LassoSelectionStep.hpp>
#include <PluginSegmentation/Steps/ManualSelectionStep.hpp>

#include <Voxie/Node/SegmentationStep.hpp>

#include <QtCore/QThreadPool>
#include <VoxieBackend/Data/HistogramProvider.hpp>
#include <VoxieBackend/IO/Operation.hpp>

#include <type_traits>

namespace vx {

enum brushModes { select, erase };

class StepManager : public QObject, public StepManagerI {
  Q_OBJECT
 public:
  StepManager(SegmentationProperties* properties, QObject* parent);
  ~StepManager();

  /**
   * @brief creates an MetaStep and starts calculation for the
   * given label id
   * @param labelID label id that shall be modified
   * @param key key of the property to modify
   * @param value new value that shall be assigned to the key
   */
  void createMetaStep(qlonglong labelID, QString key, QVariant value) override;

  /**
   * @brief adds a row to the label table by creating a meta step with the given
   * propeties
   * @param labelID label id of the row
   * @param name row name
   * @param description row description
   * @param color row box color and voxel color used in visualizers for this
   * label
   * @param visibility visibility for the display of this label in the
   * visualizer
   */
  void addLabelTableRow(qlonglong labelID, QString name, QString description,
                        QVariant color, bool visibility);

  /**
   * @brief creates an RemoveLabelStep and starts removal calculation for the
   * given label ids (removes them from the labelTable & labelVolume)
   * @param labelIDs list of labels that shall be removed
   */
  void createRemoveLabelStep(QList<SegmentationType> labelIDs);

  /**
   * @brief creates an AssignmentStep and starts calculation for the last
   * selection step to the given label id
   * @param labelID single label where the selection shall be assigned to
   */
  void createAssignmentStep(qlonglong labelID);

  /**
   * @brief creates an SubtractStep and starts calculation for the last
   * selection step to remove its selection from the given label id
   * @param labelId single label from which the selection shall be subtracted
   */
  void createSubtractStep(qlonglong labelId);

  /**
   * @brief sets ManualSelectionStep properties for label ids choosen by the
   * user via GUI and runs calculation
   * @param labelIds Vector of label ids
   */
  void setManualSelection(QList<SegmentationType> labelIds);

  /**
   * @brief Sets the BrushSelectionStep properties (Plane, Volume) of the Step
   * related to the currentSV
   * @param plane Plane in which the Brush operates
   * @param currentSV SliceVisualizer to which the brush belongs
   */
  void setBrushSelectionProperties(PlaneInfo plane,
                                   SliceVisualizerI* currentSV) override;

  /**
   * @brief Creates a new Lasso selection step. In the step the voxels inside a
   * polygon that is spanned by the nodes are selected.
   * @param nodes Nodes that span the simple Polygon
   * @param plane Plane in which the polygon is placed
   */
  void setLassoSelection(QList<vx::Vector<double, 3>> nodes, PlaneInfo plane,
                         SliceVisualizerI* currentSV) override;

  /**
   * @brief returns the selection steps that belongs to the passed
   * SliceVisualizer. First entry is the BrushSelectionStep, second:
   * LassoSelectionStep
   * @param sV Slice-Visualizer instance
   */
  QMap<QString, QSharedPointer<SegmentationStep>> getSelectionSteps(
      SliceVisualizerI* sV);

  /**
   * @brief Sets or clears the selection bit of the voxels around the center of
   * the brush. Behaviour depends on the choice of Brush or Eraser in the GUI
   * @param centerWithRadius Center of Brush [m] (global system) and Brush
   * Radius [m]
   */
  void addVoxelsToBrushSelection(
      std::tuple<vx::Vector<double, 3>, double> centerWithRadius,
      PlaneInfo plane, SliceVisualizerI* currentSV) override;

  /**
   * @brief sets the input volume data for the histograms
   * @param volumeNode pointer to volume node
   */
  void setVolume(QSharedPointer<VolumeNode> volumeNode);

  /**
   * @brief returns the output container data from the properties
   */
  QSharedPointer<ContainerData> getOutputData() const;
  /**
   * @brief returns the input volume data from the properties
   */
  QSharedPointer<VolumeNode> getInputVolume() const;

  void setBrushMode(brushModes brushMode) { this->brushMode = brushMode; }

  brushModes getBrushMode() { return this->brushMode; }

  /**
   * @brief clears the active selection bit from all voxels
   */
  void clearSelection();

  /**
   * @brief runs a list of SegmentationStep nodes sequentially
   */
  void runStepList(const QList<vx::Node*>& stepList);

  /**
   * @brief returs the histogram of the currently connected input volume
   */
  QSharedPointer<HistogramProvider> getHistogramProvider() {
    return this->histogramProvider;
  }

  /**
   * @brief Initialize the non special steps and return a reference to them
   */
  QList<QSharedPointer<vx::SegmentationStep>> spawnDefaultSteps(
      LabelViewModel* labelViewModel,
      const QItemSelectionModel* selectionModel);

  /**
   * @brief removes multiple given steps from the step list property via indices
   * @param indices List of indices for steps that shall be removed from the
   * list
   */
  void removeSteps(const QList<int>& indices);

 Q_SIGNALS:
  /**
   * @brief signal emitted when a selection operation (only setting the MSB of a
   * voxel volume)finished its calculation
   * @param status status of selection finished state, false=not finished,
   * true=finished
   */
  void selectionIsFinished(bool status);

  /**
   * @brief signal emitted when the ability to apply operations on the active
   * selection changes
   * @param status states if operations on the active selection are allowed
   */
  void canApplyActiveSelection(bool status);

  /**
   * @brief signal emitted when a voxel operation (any modification on voxels in
   * a volume) finished its calculation
   * @param status status of voxel operation finished state, false=not finished,
   * true=finished
   */
  void voxelOpIsFinished(bool status);

  /**
   * @brief signal emitted when the input volume changes
   * @param obj pointer to the volume object
   */
  void inputVolChanged(VolumeNode* obj);

  /**
   * @brief signal emitted when LabelAddStep finished on thread
   * @param status status of operation finished state, false=not finished,
   * true=finished
   */
  void labelAddIsFinished(bool status);
  /**
   * @brief signal emitted when LabelRemoveStep finished on thread
   * @param status status of operation finished state, false=not finished,
   * true=finished
   */
  void labelRemoveIsFinished(bool status);

  /**
   * @brief signal emitted when the active selection voxel count changed
   * @param voxelCount number of newly selected voxel
   * @param incremental bool to decide upgrade mode. Incremental = False is
   * total number update. Incremental = True is a difference number which shall
   * be accumulated to the existing count
   */
  void updateSelectedVoxelCount(qint64 voxelCount, bool incremental);

  /**
   * @brief Called, when all Step UI widgets should be resetted to the default
   * state
   */
  void resetUIWidgets();

 private:
  QList<QSharedPointer<SegmentationStep>> selectionStepList;
  SegmentationProperties* segmentationProperties;

  /**
   * @brief registers a step:
   *      1. Initializes the properties of the step
   *      2. Connects the update voxel count signal
   */
  template <typename T, typename = std::enable_if<
                            std::is_base_of<SegmentationStep, T>::value>>
  void registerStep(QSharedPointer<T>& step) {
    QMap<QString, QVariant> properties;
    step = qSharedPointerDynamicCast<T>(T::getPrototypeSingleton()->create(
        properties, QList<Node*>(), QMap<QString, QDBusVariant>(), false));
  }

  QSharedPointer<HistogramProvider> volumeHistogramProvider;
  QMetaObject::Connection histogramConnection;
  QSharedPointer<HistogramProvider> histogramProvider;

  /**
   * List used for threading of SegmentationStep caluclations where they are
   * queued until execution
   */
  QList<QSharedPointer<SegmentationStep>> executionQueue;
  QSharedPointer<SegmentationStep> runningStep;

  bool isCalcAutomatic = true;
  brushModes brushMode = brushModes::select;

  /**
   * @brief adds a given SegmentationStep to the step list property
   * @param node pointer to SegmentationStep that shall be added
   */
  void addStepToList(QSharedPointer<vx::SegmentationStep> node) const;

  /**
   * @brief gets, converts and returns the label volume from the output compound
   * @param object pointer to SegmentationStep that shall be added
   * @return pointer to VolumeDataVoxelInst of label volume from compound
   */
  const QSharedPointer<VolumeDataVoxelInst<SegmentationType>> getLabelVolume()
      const {
    if (!isDataAvailable()) {
      return QSharedPointer<VolumeDataVoxelInst<SegmentationType>>();
    } else {
      return qSharedPointerDynamicCast<VolumeDataVoxelInst<SegmentationType>>(
          getOutputData()->getElement("labelVolume"));
    }
  }

  /**
   * @brief Clears the internal stepList
   */
  void clearSelectionStepList();

  /**
   * @brief check segmentation input and output for nullpointers and emit
   * warnings if so
   * @return boolean of data availability, true=all data available, else false
   */
  bool isDataAvailable() const;

  /**
   * @brief calculates SegmentationStep from the executionQueue sequentially
   * threaded
   */
  void runStep();

  /**
   * @brief determines and starts the execution method and signals for given
   * step
   * @param step step to be executed
   */
  void runStepByKind(QSharedPointer<SegmentationStep> step);

  // member filter steps instances
  QSharedPointer<ManualSelectionStep> manualSelection;

 public:
  // debug functions to print step list & selection step list
  void printList();
  void printSelectionStepList();

  QSharedPointer<BrushSelectionStep> brushSelectionXY;
  QSharedPointer<BrushSelectionStep> brushSelectionXZ;
  QSharedPointer<BrushSelectionStep> brushSelectionYZ;

  QSharedPointer<LassoSelectionStep> lassoSelectionXY;
  QSharedPointer<LassoSelectionStep> lassoSelectionXZ;
  QSharedPointer<LassoSelectionStep> lassoSelectionYZ;
};
}  // namespace vx
