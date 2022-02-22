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

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFunctions_3_2_Compatibility>

#include <QtWidgets/QOpenGLWidget>

#include <osvr/RenderKit/RenderManager.h>

namespace osvr {
namespace clientkit {
class ClientContext;
}
namespace renderkit {
class RenderManager;
}
}  // namespace osvr

class Qt5ToolkitImpl;

class OsvrDisplay : public QObject, public QOpenGLFunctions_3_2_Compatibility {
  Q_OBJECT

  QOpenGLContext* context;

  QSharedPointer<osvr::clientkit::ClientContext> osvrContext;
  QSharedPointer<osvr::renderkit::RenderManager> renderManager;

  std::vector<osvr::renderkit::RenderBuffer> colorBuffers;
  std::vector<GLuint> depthBuffers;

  GLuint frameBuffer;

  Qt5ToolkitImpl* toolkit;

  QTimer* timer;

  bool initialize();

  void updateDisplay();
  bool initialized = false;
  bool initializeFailed = false;

 public:
  OsvrDisplay(QOpenGLContext* context, QObject* parent);
  ~OsvrDisplay();

  void resetYaw();

 Q_SIGNALS:
  void render(const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix);
};
