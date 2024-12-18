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
#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/ContainerNode.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/SegmentationStep.hpp>

namespace vx {

/*! SubtractStep
 * SegmentationStep which subtracts an existing selection in labelVolume from
 * the given labelIDs. Iterates through all labelVolume voxels and compares if
 * they are selected and fitting the given labelID. If true they are set to 0.
 * Additionally removes any existing selection not fitting the label ID.
 */
class SubtractStep : public SegmentationStep {
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.SegmentationStep.SubtractStep")

 private:
  void initializeCustomUIPropSections() override;

 public:
  SubtractStep(qlonglong labelID);
  SubtractStep();
  ~SubtractStep();

  QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) override;

  QString getInfoString() override;

  void resetUIWidget() override;

  void onVoxelOpFinished(bool status) override;

  QList<QString> supportedDBusInterfaces() override;

  bool isAllowedChild(NodeKind object) override;
  bool isAllowedParent(NodeKind object) override;
  bool isCreatableChild(NodeKind object) override;
};

}  // namespace vx
