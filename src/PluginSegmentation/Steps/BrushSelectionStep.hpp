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
#include <PluginSegmentation/Prototypes.forward.hpp>
#include <PluginSegmentation/Prototypes.hpp>
#include <PluginSegmentation/SegmentationUtils.hpp>
#include <QtCore/QSharedPointer>
#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>
#include <Voxie/Interfaces/SliceVisualizerI.hpp>
#include <Voxie/Node/SegmentationStep.hpp>
#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieClient/Array.hpp>
#include <set>

namespace vx {
/*! BrushSelectionStep
 * SegmentationStep for selecting voxels by brush tool. Takes the given
 * brush centers and calculates the voxel indices which lie inside the brush
 * radius. Sets the selection bit for these voxel indices that were calculated.
 */
class BrushSelectionStep : public SegmentationStep {
  NODE_PROTOTYPE_DECL(BrushSelectionStep)

 private:
  void initializeCustomUIPropSections() override;

 public:
  BrushSelectionStep(
      QList<std::tuple<QVector3D, double>> selectCentersWithRadiuses,
      QList<std::tuple<QVector3D, double>> eraseCentersWithRadiuses,
      QVector3D volumeOrigin, QQuaternion volumeOrientation,
      QVector3D voxelSize, PlaneInfo plane);
  BrushSelectionStep(const BrushSelectionStep& oldStep);
  BrushSelectionStep();

  ~BrushSelectionStep();

  // needed to match the SliceVisualizer to the corresponding step
  SliceVisualizerI* visualizer;

  SliceVisualizerI* getVisualizer() { return this->visualizer; }

  void setVisualizer(SliceVisualizerI* sv) { this->visualizer = sv; }

  /**
   * @brief sets the selection bit of voxels in select list and clears selection
   * bit of voxels in erase list
   * @param containerData containerData instance that contains the labelVolume
   */
  QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) override;

  /**
   * @brief Sets the selection bit of the voxels around the center of
   * the brush inside the BrushRadius.
   * @param containerData containerData instance that contains the labelVolume
   * @param centerWithRadius Center of Brush [m] (global system) and Brush
   * Radius [m]
   **/
  QSharedPointer<vx::io::Operation> selectPassedVoxels(
      std::tuple<QVector3D, double> centerWithRadius,
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QSharedPointer<vx::io::Operation> op);

  /**
   * @brief Clears the selection bit of the voxels around the center of
   * the brush inside the BrushRadius.
   * @param containerData containerData instance that contains the labelVolume
   * @param centerWithRadius Center of Brush [m] (global system) and Brush
   * Radius [m]
   **/
  QSharedPointer<vx::io::Operation> erasePassedVoxels(
      std::tuple<QVector3D, double> centerWithRadius,
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QSharedPointer<vx::io::Operation> op);

  QString getInfoString() override;

  void setProperties(
      QList<std::tuple<QVector3D, double>> selectCentersWithRadiuses,
      QList<std::tuple<QVector3D, double>> eraseCentersWithRadiuses,
      QVector3D volumeOrigin, QQuaternion volumeOrientation,
      QVector3D voxelSize, QVector3D planeOrigin, QQuaternion planeOrientation);

  void setPlaneProperties(PlaneInfo plane);

  BrushSelectionStepProperties* getProperties() { return this->properties; };

  void resetUIWidget() override;

  void onVoxelOpFinished(bool status) override;

  bool isAllowedChild(NodeKind object) override;
  bool isAllowedParent(NodeKind object) override;
  bool isCreatableChild(NodeKind object) override;
  QList<QString> supportedDBusInterfaces() override;

 private:
  /**
   * @brief Append single select center
   * @param centerWithRadius centerWithRadius Center of Brush [m] (global
   system) and Brush
   * Radius [m]
   **/
  void addEraseCenterWithRadius(std::tuple<QVector3D, double> centerWithRadius);
  /**
 * @brief Append single erase center
 * @param centerWithRadius centerWithRadius Center of Brush [m] (global
 system) and Brush
 * Radius [m]
 **/
  void addSelectCenterWithRadius(
      std::tuple<QVector3D, double> centerWithRadius);
};

/**
 * @brief The BrushCalculator class provides all the functionality to calculate
 * the voxel indices that are in a certain radius next to a central pixel &
 * overlap with a plane/ are really close to it
 */
class BrushCalculator : public IndexCalculator {
 public:
  /**
   * @param brushCenter brush center in voxel coordinates [m]
   * @param brushRadius brush radius [m]
   **/
  BrushCalculator(QVector3D brushCenter, double brushRadius,
                  QSharedPointer<VolumeDataVoxel> originalVolume,
                  QVector3D planeOrigin, QQuaternion planeOrientation);
  ~BrushCalculator() {}

 private:
  // Brush radius [m]
  double brushRadius;
  double brushRadiusSquared;

  // Brush center in global coordinates [m]
  QVector3D brushCenter;

  // Brush center on plane [m]
  QPointF brushCenterOnPlane;

  /**
   * @brief Check if a voxel is inside or on the brushRadius
   * @param voxelIndex voxel that should be checked
   * @return boolen: True-> is inside, False -> is outside
   */
  bool inline isVoxelInsideBrushRadius(voxel_index currentVoxel);

  /**
   * @brief Check if a voxel lays inside the brushRadius and if it lays on
   * the plane/ is really close to it
   * @param voxelIndex voxel that should be checked
   * @return boolen: True-> criteria met, else: False
   */
  bool areCriteriaMet(voxel_index& voxelIndex) override;

 public:
  bool inline isCenterPixelInVolume();
};

}  // namespace vx
