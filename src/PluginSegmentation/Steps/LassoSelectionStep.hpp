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
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>
#include <VoxieClient/Array.hpp>
#include <set>

namespace vx {
/*! LassoSelectionStep
 * SegmentationStep for selecting voxels by polygon tool. Takes the given
 * polygon nodes and calculates the voxel indices which lie inside the spanning
 * polygon. Sets the selection bit for these voxel inidices that were
 * calculated.
 */
class LassoSelectionStep : public SegmentationStep {
  VX_NODE_IMPLEMENTATION(
      "de.uni_stuttgart.Voxie.SegmentationStep.LassoSelectionStep")

 private:
  void initializeCustomUIPropSections() override;

 public:
  LassoSelectionStep(QList<vx::Vector<double, 3>> nodes, QVector3D volumeOrigin,
                     QQuaternion volumeOrientation, QVector3D voxelSize,
                     PlaneInfo plane);
  LassoSelectionStep(const LassoSelectionStep& oldStep);
  LassoSelectionStep();

  ~LassoSelectionStep();

  // needed to match the SliceVisualizer to the corresponding step
  SliceVisualizerI* visualizer;

  SliceVisualizerI* getVisualizer() { return this->visualizer; }

  void setVisualizer(SliceVisualizerI* sv) { this->visualizer = sv; }

  /**
   * @brief calculates the indices of all voxels inside the polygons and selects
   * them
   * @param containerData containerData instance that contains the labelVolume
   */
  QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) override;

  QString getInfoString() override;

  void resetUIWidget() override;

  void onVoxelOpFinished(bool status) override;

  void setProperties(QList<vx::Vector<double, 3>> nodes, QVector3D volumeOrigin,
                     QQuaternion volumeOrientation, QVector3D voxelSize,
                     QVector3D planeOrigin, QQuaternion planeOrientation);

  LassoSelectionStepProperties* getProperties() { return this->properties; };

  bool isAllowedChild(NodeKind object) override;
  bool isAllowedParent(NodeKind object) override;
  bool isCreatableChild(NodeKind object) override;
  QList<QString> supportedDBusInterfaces() override;
};

/**
 * @brief The LassoCalculator class provides all the functionality to calculate
 * the voxel indices that are inside a simple polygon (without holes, no
 * intersection)& overlap with a plane/ are really close to it
 */
class LassoCalculator : public IndexCalculator {
 public:
  LassoCalculator(QList<vx::Vector<double, 3>> nodes,
                  QSharedPointer<VolumeDataVoxel> originalVolume,
                  QVector3D planeOrigin, QQuaternion planeOrientation);
  ~LassoCalculator() {}

 private:
  QList<QPointF>
      nodesPlane;  // all points that span the simple polygon, in
                   // plane coordinates, last and first point are connected

  QRectF boundingBox;  // bounding box of the nodes

  /**
   * @brief Check if a voxel lays inside of the simple polygon and if it lays on
   * the plane/ is really close to it
   * @param voxelIndex voxel that should be checked
   * @return boolen: True-> criteria met, else: False
   */
  bool areCriteriaMet(voxel_index& voxelIndex) override;

  /**
   * @brief Check if a voxel lays inside of the simple polygon that is spanned
   * by the nodes
   * @param voxelIndex voxel that should be checked
   * @return boolen: True-> is inside, False -> is outside
   */
  bool inline isVoxelInsidePolygon(voxel_index& voxelIndex);

  /**
   * @brief calculates the points on an ellipsoid with a spacing of xShift &
   * yShift in between. Ellipsoid: (x**2/a**2 + y**2/b**2 = 1)
   * @param center center of ellipsoid
   * @param radius radius if ellipsoid would be a circle, e.g.  a=b
   * @param xShift Space between points in x direction: radius*xShift = a,
   * @param yShift Space between points in y direction: radius*yShift = b,
   * @return List with points on Ellipsoid
   */
  QList<QPointF> getPointsOnEllipse(QPointF center, int radius, float xShift,
                                    float yShift);

  /**
   * @brief Calculates the boundingBox of all nodes & expands the box by
   * 10*pixelSize in the respective diretion
   * @return expanded boundingBox
   */
  QRectF getBoundingBox();
};

}  // namespace vx
