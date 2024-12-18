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
#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/ContainerNode.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/SegmentationStep.hpp>

namespace vx {
/*! AssignmentStep
 * SegmentationStep which adds an existing selection in labelVolume to the given
 * labelId. Iterates through all labelVolume voxels and sets the voxels with the
 * label ID value where the selection bit is set. Also the selection bit for the
 * voxels is cleared.
 */
class AssignmentStep : public SegmentationStep {
  VX_NODE_IMPLEMENTATION(
      "de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep")

 private:
  void initializeCustomUIPropSections() override;

  /* data */
 public:
  AssignmentStep(qlonglong labelID);
  AssignmentStep();
  ~AssignmentStep();

  QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) override;

  QString getInfoString() override;

  void resetUIWidget() override;

  void onVoxelOpFinished(bool status) override;

  /**
   * @brief add the current active selection voxels (with MSB==1) to the
   * selected row of the table view
   * @param labelData pointer to the output labelVolume
   */
  void addSelection(QSharedPointer<ContainerData> containerData, int labelID,
                    QSharedPointer<vx::io::Operation> op);

  QList<QString> supportedDBusInterfaces() override;

  bool isAllowedChild(NodeKind node) override;
  bool isAllowedParent(NodeKind node) override;
  bool isCreatableChild(NodeKind node) override;
};

}  // namespace vx
