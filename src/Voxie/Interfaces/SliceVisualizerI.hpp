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

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Voxie.hpp>
#include <VoxieBackend/Data/HistogramProvider.hpp>

namespace vx {

class VOXIECORESHARED_EXPORT SliceVisualizerI {
 public:
  SliceVisualizerI();
  virtual ~SliceVisualizerI();

  /**
   * @brief Sets the brush selection tool as currently active tool
   */
  virtual void activateBrushSelectionTool() = 0;

  /**
   * @brief Set the current tool to SliceAdjustment if BrushSelection is active
   */
  virtual void deactivateBrushSelectionTool() = 0;

  /**
   * @brief Sets the lasso selection tool as currently active tool
   */
  virtual void activateLassoSelectionTool() = 0;

  /**
   * @brief Set the current tool to SliceAdjustment if LassoSelection is active
   */
  virtual void deactivateLassoSelectionTool() = 0;

  /**
   * @brief Sets the brush Radius in pixels
   */
  virtual void setBrushRadius(quint8 radius) = 0;

  /**
   * @brief Returns the histogram provider of slice visualizer histogram widget
   */
  virtual QSharedPointer<vx::HistogramProvider> getSliceHistogramProvider() = 0;

  /**
   * @brief Sets the histogram provider of slice visualizer histogram widget
   */
  virtual void setHistogramColorizer(
      QSharedPointer<vx::Colorizer> colorizer) = 0;

  /**
   * @brief Get the colorizer of slice visualizer histogram widget
   */
  virtual QSharedPointer<vx::Colorizer> getHistogramColorizer() = 0;

  /**
   * @brief Change the volume to be displayed
   */
  virtual void setVolume(vx::Node* volume) = 0;

  /**
   * @brief Change the plane rotation
   */
  virtual void setRotation(QQuaternion rotation) = 0;

  /**
   * @brief returns the current Brush radius in pixels
   */
  virtual quint8 getBrushRadius() = 0;
};
}  // namespace vx
