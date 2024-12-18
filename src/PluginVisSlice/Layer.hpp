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

#include <VoxieBackend/Data/SliceImage.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QObject>

#include <QtWidgets/QWidget>

namespace vx {
class ParameterCopy;
}  // namespace vx

class Layer : public vx::RefCountedObject {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  Layer();
  ~Layer();

 public:
  virtual QString getName() = 0;
  virtual QString displayName();

 public:
  // Should be able to run in a background thread
  // This function is passed a QImage instead of a QPainter to avoid changes to
  // QPainter settings in one layer affecting painting in another layer.
  virtual void render(QImage& outputImage,
                      const QSharedPointer<vx::ParameterCopy>& parameters,
                      bool isMainImage) = 0;

  /**
   * Trigger a redraw operation, which will be delayed until the code returns
   * to the main loop. Multiple triggerRedraw() calls before the redraw is
   * actually executed will lead to only one redraw operation.
   */
  void triggerRedraw();

  // TODO: is this really needed or should this just simply trigger a redraw?
  /**
   * @brief onResize signals that the canvas size the slice visualizer uses has
   * been changed.
   */
  void onResize(const QSize& size);

  // Returns true if there is no redraw pending or running
  bool isUpToDate();

 private:
  void maybeStartRedraw();
  void maybeUpdateIsUpToDate();
  void doRedraw(const QSharedPointer<vx::ParameterCopy>& parameters,
                const QSize& size);

  void setResultImage(const QImage& image);
  void clearResultImage();

 Q_SIGNALS:
  void resultImageChanged(const QImage& image);
  void getRenderingParameters(QSharedPointer<vx::ParameterCopy>& parameters,
                              QSize& size);
  void isUpToDateChanged();

 private:
  // redrawRequested is set to true when triggerRedraw() has been called and a
  // new redraw operation has to be started
  bool redrawRequested = false;
  // redrawPending is set to true when the redraw is waiting for a return to the
  // main loop (in order to combine multiple requests)
  bool redrawPending = false;
  // redrawRunning is set to true while a redraw operation is actually running
  bool redrawRunning = false;

  bool isUpToDate_ = false;

  // Not used because with multithreaded rendering reusing the existing image
  // doesn't work anyway
  // QImage cachedImage;
};
