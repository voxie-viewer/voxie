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

#include <VoxieBackend/Component/ComponentTypeInfo.hpp>
#include <VoxieBackend/Component/ExternalOperation.hpp>

#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/IO/RunFilterOperation.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <Voxie/Node/SegmentationStep.hpp>

namespace vx {
template <typename T>
class SharedFunPtr;
class PropertyBase;

class ExtensionSegmentationStep : public SegmentationStep {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExtensionSegmentationStep)

  QString scriptFilename_;
  QString infoString;

 private:
  void initializeCustomUIPropSections() override;

 public:
  ExtensionSegmentationStep(const QSharedPointer<vx::NodePrototype>& prototype,
                            const QString& scriptFilename);
  ~ExtensionSegmentationStep();

  const QString& scriptFilename() const { return scriptFilename_; }

  QSharedPointer<OperationResult> calculate(
      QSharedPointer<ParameterCopy> parameterCopy,
      QSharedPointer<ContainerData> containerData,
      QDBusObjectPath inputVolume) override;

  QString getInfoString() override;

  void resetUIWidget() override;

  void onVoxelOpFinished(bool status) override;

  QList<QString> supportedDBusInterfaces() override;

  bool isAllowedChild(NodeKind node) override;
  bool isAllowedParent(NodeKind node) override;
  bool isCreatableChild(NodeKind node) override;

 Q_SIGNALS:
  void error(const Exception& e, const QSharedPointer<QString>& scriptOutput);
};

class ExternalOperationRunSegmentationStepAdaptorImpl;

class ExternalOperationRunSegmentationStep : public ExternalOperation {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(ExternalOperationRunSegmentationStep)

  friend class ExternalOperationRunSegmentationStepAdaptorImpl;

  QSharedPointer<vx::io::Operation> operation_;

  QSharedPointer<vx::SegmentationStep> segmentationStep_;

  QDBusObjectPath SegmentationStepPath_;

  QSharedPointer<vx::ContainerData> containerData_;

  QDBusObjectPath inputVolume_;

  QSharedPointer<vx::NodePrototype> prototype_;

  QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> parameters_;

  // A list of objects which are kept alive while the operation is running
  QSharedPointer<QList<QSharedPointer<vx::ExportedObject>>> references;

 protected:
  void cleanup() override;

 public:
  explicit ExternalOperationRunSegmentationStep(
      const QSharedPointer<vx::io::Operation>& operation,
      const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& parameters,
      const QSharedPointer<QList<QSharedPointer<vx::ExportedObject>>>&
          references,
      QSharedPointer<vx::SegmentationStep> step,
      QSharedPointer<vx::ContainerData> containerData,
      QDBusObjectPath inputVolume);
  ~ExternalOperationRunSegmentationStep();

  const QSharedPointer<vx::io::Operation>& operation() const {
    return operation_;
  }

  QString action() override;

  QString name() override;

  const QSharedPointer<vx::SegmentationStep>& SegmentationStep() {
    return this->segmentationStep_;
  }

  const QSharedPointer<vx::ContainerData>& containerData() {
    return this->containerData_;
  }

  const QDBusObjectPath& inputVolume() { return this->inputVolume_; }

  const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& parameters() {
    return this->parameters_;
  }
};

}  // namespace vx
