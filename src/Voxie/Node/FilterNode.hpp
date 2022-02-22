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

#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>

#include <Voxie/Data/PreviewBox.hpp>
#include <Voxie/IVoxie.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/WeakParameterCopy.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <Voxie/IO/RunFilterOperation.hpp>

using namespace vx;

namespace vx {
class ExtensionFilterNode;

class VOXIECORESHARED_EXPORT FilterNode : public vx::Node {
  Q_OBJECT

 public:
  FilterNode(const QSharedPointer<NodePrototype>& prototype);
  virtual ~FilterNode();

  QList<QString> supportedDBusInterfaces() override;

  /**
   * @brief Starts the external script.
   */
  QSharedPointer<vx::io::RunFilterOperation> run();

  bool isAllowedChild(NodeKind kind) override;
  bool isAllowedParent(NodeKind kind) override;
  bool isCreatableChild(NodeKind kind) override;

  /**
   * @brief True when the filter's properties have changed since the last run
   * and it thus needs a recalculation. False otherwise.
   */
  bool needsRecalculation();

  PreviewBox* previewBox() { return this->previewBox_; }

 private:
  /**
   * @brief Internal method that is executed when the filter is run. Should be
   * overwritten with a concrete implementation by the filter.
   */
  // TODO: create a subclass for built-in filters which handles the
  // go-to-background-thread stuff
  virtual QSharedPointer<vx::io::RunFilterOperation> calculate() = 0;

  /**
   * @brief Updates the calculate button's enable/disable state depending on the
   * current state of the filter node (e. g. are all required parents
   * connected).
   * @return returns true when the button should be enabled, otherwise false.
   */
  void updateCalculateButtonState();

  QWidget* mainPropertyWidget;

  QLabel* displayLabel;
  QList<QMetaObject::Connection> displayLabelConnections;

  /**
   * @brief Called whenever parents or children change to update the labels.
   */
  void displayLabelParentChildChanged();
  /**
   * @brief Called whenever the name of a parent or child changes.
   */
  void updateDisplayLabel();

 private:
  QPushButton* calculateButton;
  QToolButton* stopProcessButton;

  PreviewBox* previewBox_ = new PreviewBox(QVector3D(), QVector3D(), false);
  // VolumeNode* previewVolumeNode = nullptr;

 protected:
  QSharedPointer<WeakParameterCopy> runParameters;
  QLabel* sourceCodeButton;
  QCheckBox* debuggerSupportEnabled;
};
}  // namespace vx
