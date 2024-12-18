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

#include <Voxie/Voxie.hpp>

#include <Voxie/Data/TomographyRawDataNode.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <Voxie/IVoxie.hpp>

#include <Voxie/Node/Node.hpp>

#include <VoxieClient/SharedFunPtr.hpp>

#include <QtCore/QVector>

#include <QtDBus/QDBusAbstractAdaptor>

#include <QtWidgets/QWidget>

namespace vx {
// Forward declarations
class VolumeNode;
class ImageDataPixel;
class ParameterCopy;

class VOXIECORESHARED_EXPORT VisualizerRenderOptions {
  bool isMainView_;

 public:
  VisualizerRenderOptions(bool isMainView);
  ~VisualizerRenderOptions();

  /**
   * @brief True if this render call is for rendering the main view of the
   * visualizer (ant not e.g. for rendering a screenshot).
   */
  bool isMainView() const { return isMainView_; }
};

/**
 * @brief A visualizer that can show any kind of voxel data.
 */
class VOXIECORESHARED_EXPORT VisualizerNode : public vx::Node {
  Q_OBJECT
 private:
  QVector<QWidget*> sections;

 public:
  explicit VisualizerNode(const QSharedPointer<vx::NodePrototype>& prototype);
  virtual ~VisualizerNode();

  QList<QString> supportedDBusInterfaces() override;

  /**
   * @brief Returns a set of dynamic sections that will be shown/hidden
   * depending on the visualizer state.
   * @return Vector with the side panel sections.
   */
  // TODO: Remove this, this the same for VisualizerNode as for other Nodes.
  QVector<QWidget*>& dynamicSections();

  virtual QWidget* mainView() = 0;

  using RenderFunction =
      void(const QSharedPointer<vx::ImageDataPixel>&, const vx::VectorSizeT2&,
           const vx::VectorSizeT2&, const QSharedPointer<vx::ParameterCopy>&,
           const QSharedPointer<vx::VisualizerRenderOptions>&);

  /**
   * @ brief Return a function for rendering the visualizer content.
   *
   * The returned function might be called on a background thread and might be
   * called after the visualizer has been destroyed (or the visualizer might be
   * destroyed while the returned function is running).
   */
  virtual SharedFunPtr<RenderFunction> getRenderFunction();

  /**
   * Return the size of the main content area (the part which will be included
   * in a screenshot, without any buttons).
   *
   * The default implementation returns the size of mainView().
   */
  virtual QSize contentAreaSize();
  void saveScreenshot();

  bool isAllowedChild(NodeKind kind) override;
  bool isAllowedParent(NodeKind kind) override;
};
}  // namespace vx
