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

#include <qformlayout.h>
#include <qspinbox.h>
#include <qtablewidget.h>
#include <Main/Root.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Node/FilterNode.hpp>

#include <Voxie/IO/RunFilterOperation.hpp>

#include <PluginSegmentation/Gui/HistoryViewModel.hpp>
#include <PluginSegmentation/Gui/SegmentationWidget.hpp>
#include <PluginSegmentation/Prototypes.forward.hpp>
#include <PluginSegmentation/StepManager.hpp>
#include <Voxie/Data/LabelViewModel.hpp>

//#include <Main/PluginConnectionUtils.hpp>
#include <Voxie/Interfaces/SegmentationI.hpp>
#include <Voxie/Interfaces/SliceVisualizerI.hpp>

#include <Voxie/Data/ContainerNode.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <QtDBus/QDBusAbstractAdaptor>

namespace vx {
class PropertySection;

enum sliceVisOrientation { XY, XZ, YZ };

namespace filters {

class Segmentation : public FilterNode, public SegmentationI {
  NODE_PROTOTYPE_DECL(Segmentation)

 public:
  Segmentation();

  QSharedPointer<vx::io::RunFilterOperation> calculate() override;
  StepManager* getStepManager() override;
  void deactivateBrushes() override;
  void deactivateLasso() override;
  void updatePosition(QVector3D position) override;

  // needed to allow connection to visualizers
  bool isAllowedChild(vx::NodeKind object) override;

  bool isCreatableChild(NodeKind object) override;

 private:
  SliceVisualizerI* SliceVisualizerXY = nullptr;
  SliceVisualizerI* SliceVisualizerYZ = nullptr;
  SliceVisualizerI* SliceVisualizerXZ = nullptr;
  bool isChangingBrushes = false;
  bool isChangingLasso = false;

  LabelViewModel* labelViewModel;
  HistoryViewModel* historyViewModel;
  SegmentationWidget* segmentationWidget;
  StepManager* stepManager = nullptr;

  ContainerNode* labelContainer = nullptr;
  QSharedPointer<TableData> labelTablePointer = QSharedPointer<TableData>();
  vx::VectorSizeT3 lastDimensions = vx::VectorSizeT3(0, 0, 0);
  const QQuaternion rotationXZ =
      QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 90);
  const QQuaternion rotationXY =
      QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 0.0);
  const QQuaternion rotationYZ =
      QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 90);

  /**
   * @brief Spawns 3-Slice Visualizers. One for the XY, XZ & YZ plane
   */
  void spawnSliceVisualizers();
  /**
   * @brief Spawns a single slice Visualizer
   * @param planeOrientation Orientation of the plane that should be visualized
   * @param guiPosition Position where the Visualizer is placed
   */
  SliceVisualizerI* spawnSingleSliceVisualizer(QQuaternion planeOrientation,
                                               QVector2D guiPosition,
                                               QVector2D size,
                                               QString manualDisplayName);
  /**
   * @brief Configure a slice visualizer by e.g. setting its position and adding
   * the required Qt connections
   */
  void configureSliceVisualizer(vx::VisualizerNode* sliceVisualizer,
                                vx::SliceVisualizerI* interf,
                                QVector2D guiPosition, QVector2D size);
  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;
  void initData();
};
}  // namespace filters
}  // namespace vx
