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

#include "OsvrDisplay.hpp"

#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>

#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLWindow>

#include <VoxieClient/Exception.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <osvr/ClientKit/Context.h>

#include <osvr/RenderKit/RenderManager.h>

#include <GL/gl.h>
#include <osvr/RenderKit/GraphicsLibraryOpenGL.h>

#include <signal.h>

#include <osvr/RenderKit/RenderManagerOpenGL.h>
#include <osvr/RenderKit/RenderManagerOpenGLC.h>

class Qt5ToolkitImpl {
  OSVR_OpenGLToolkitFunctions toolkit;

  static void createImpl(void* data) { Q_UNUSED(data); }
  static void destroyImpl(void* data) { delete ((Qt5ToolkitImpl*)data); }
  static OSVR_CBool addOpenGLContextImpl(void* data,
                                         const OSVR_OpenGLContextParams* p) {
    return ((Qt5ToolkitImpl*)data)->addOpenGLContext(p);
  }
  static OSVR_CBool removeOpenGLContextsImpl(void* data) {
    return ((Qt5ToolkitImpl*)data)->removeOpenGLContexts();
  }
  static OSVR_CBool makeCurrentImpl(void* data, size_t display) {
    return ((Qt5ToolkitImpl*)data)->makeCurrent(display);
  }
  static OSVR_CBool swapBuffersImpl(void* data, size_t display) {
    return ((Qt5ToolkitImpl*)data)->swapBuffers(display);
  }
  static OSVR_CBool setVerticalSyncImpl(void* data, OSVR_CBool verticalSync) {
    return ((Qt5ToolkitImpl*)data)->setVerticalSync(verticalSync);
  }
  static OSVR_CBool handleEventsImpl(void* data) {
    return ((Qt5ToolkitImpl*)data)->handleEvents();
  }

  QOpenGLContext* shareContext;
  QObject* objectToDelete;

  QList<QOpenGLWindow*> glwidgets;
  // QList<QWidget*> dialogs;
  QList<QWindow*> dialogs;

 public:
  Qt5ToolkitImpl(QOpenGLContext* shareContext, QObject* objectToDelete) {
    this->shareContext = shareContext;
    this->objectToDelete = objectToDelete;

    memset(&toolkit, 0, sizeof(toolkit));
    toolkit.size = sizeof(toolkit);
    toolkit.data = this;

    toolkit.create = createImpl;
    toolkit.destroy = destroyImpl;
    toolkit.addOpenGLContext = addOpenGLContextImpl;
    toolkit.removeOpenGLContexts = removeOpenGLContextsImpl;
    toolkit.makeCurrent = makeCurrentImpl;
    toolkit.swapBuffers = swapBuffersImpl;
    toolkit.setVerticalSync = setVerticalSyncImpl;
    toolkit.handleEvents = handleEventsImpl;
  }

  ~Qt5ToolkitImpl() {}

  const OSVR_OpenGLToolkitFunctions* getToolkit() const { return &toolkit; }

  // const QList<QWidget*>& getDialogs() const { return dialogs; }
  const QList<QWindow*>& getDialogs() const { return dialogs; }

  bool addOpenGLContext(const OSVR_OpenGLContextParams* p) {
    // auto dialog = new QDialog();
    // auto dialog = new QWidget();

    /*
    qDebug() << "Size:" << p->width << p->height;
    qDebug() << "Pos:" << p->xPos << p->yPos;
    //qDebug() << "Dis:" << p->displayIndex << p->fullScreen;
    qDebug() << "Dis:" << p->fullScreen;
    // */

    /*
    auto layout = new QHBoxLayout(dialog);
    layout->setMargin(0);
    layout->setSpacing(0);
    */
    auto sc = glwidgets.size() ? glwidgets.at(0)->context() : shareContext;
    auto gl = new QOpenGLWindow(sc);
    auto dialog = gl;  // TODO: check this
    // layout->addWidget(gl);

    // TODO: This doesn't work because gl->context() isn't set here yet. Check
    // later that sharing was set up properly.
    /*
    if (!QOpenGLContext::areSharing(sc, gl->context())) {
      delete dialog;
      qCritical() << "Setting up OpenGL sharing failed";
      return false;
    }
    */

    // dialog->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    dialog->setFlags(Qt::Window | Qt::FramelessWindowHint);

    dialog->setGeometry(p->xPos, p->yPos, p->width, p->height);

    // dialog->setWindowTitle(p->windowTitle);
    dialog->setTitle(p->windowTitle);

    if (p->fullScreen) dialog->setWindowState(Qt::WindowFullScreen);

    if (objectToDelete) {
      QObject::connect(dialog, &QObject::destroyed, objectToDelete,
                       &QObject::deleteLater);
      /*
     QObject::connect(dialog, &QDialog::finished, objectToDelete,
                      &QObject::deleteLater);
      */
    }

    if (p->visible) dialog->show();

    // Make sure the window is actually shown. This is needed because OSVR
    // expects the OpenGL context to be available immediately and QOpenGLWindow
    // will create it only when this dialog is shown
    dialog->show();
    QCoreApplication::processEvents();
    if (!gl->context() || !gl->context()->isValid()) {
      qWarning() << "OSVR: OpenGL context not set after showing dialog";
      gl->deleteLater();
      return false;
    }

    // qDebug() << dialog->geometry();

    glwidgets.push_back(gl);
    dialogs.push_back(dialog);

    return true;
  }
  bool removeOpenGLContexts() {
    for (auto dialog : dialogs) dialog->deleteLater();
    dialogs.clear();
    glwidgets.clear();

    return true;
  }
  bool makeCurrent(size_t display) {
    // qDebug() << "makeCurrent" << display << glwidgets.at(display)->context();
    if (!glwidgets.at(display)->context()) {
      qWarning() << "OSVR makeCurrent() failed: No context";
      return false;
    }
    glwidgets.at(display)->makeCurrent();
    return true;
  }
  bool swapBuffers(size_t display) {
    auto window = glwidgets.at(display);
    auto context = window->context();
    if (!context) {
      qCritical() << "window->context() is nullptr";
      return false;
    }
    context->swapBuffers(window);
    return true;
  }
  bool setVerticalSync(bool verticalSync) {
    Q_UNUSED(verticalSync);  // TODO: ?
    return true;
  }
  bool handleEvents() { return true; }

  bool isContextValid(size_t display) {
    return glwidgets.at(display)->context() &&
           glwidgets.at(display)->context()->isValid();
  }
};

static QString getGLErrorString(GLenum error) {
  switch (error) {
    case GL_NO_ERROR:
      return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    default:
      return QString::number(error);
  }
}
static bool checkOpenGLStatus() {
  GLenum error = glGetError();
  if (error == GL_NO_ERROR) return true;

  QString errorStr;
  int count = 0;
  while (error != GL_NO_ERROR) {
    if (count != 0) errorStr += ",";
    errorStr += getGLErrorString(error);

    if (count > 20)
      break;  // Prevent endless loop if glGetError() doesn't clear the error

    error = glGetError();
    count++;
  }
  qWarning() << "Got OpenGL error:" << errorStr;
  return false;
}

void videoStatusCallback(void* userdata, const OSVR_TimeValue* timestamp,
                         const OSVR_AnalogReport* report) {
  Q_UNUSED(userdata);
  Q_UNUSED(timestamp);

  int newStatus = (int)report->state;
  static int oldStatus = -1;
  if (newStatus == oldStatus) return;
  oldStatus = newStatus;

  if (newStatus == 0)
    qDebug() << "New OSVR video status: Unknown";
  else if (newStatus == 1)
    qDebug() << "New OSVR video status: NoInput";
  else if (newStatus == 2)
    qDebug() << "New OSVR video status: Portrait";
  else if (newStatus == 3)
    qDebug() << "New OSVR video status: Landscape";
  else
    qDebug() << "New OSVR video status: Unknown value:" << newStatus;
}

OsvrDisplay::OsvrDisplay(QOpenGLContext* context, QObject* parent)
    : QObject(parent), context(context) {
  osvrContext = createQSharedPointer<osvr::clientkit::ClientContext>(
      "de.uni_stuttgart.Voxie");

  osvr::clientkit::Interface hdkVideoStatus = osvrContext->getInterface(
      "/com_osvr_Multiserver/OSVRHackerDevKit0/semantic/status/videoStatus");

  hdkVideoStatus.registerCallback(&videoStatusCallback, NULL);

  toolkit = new Qt5ToolkitImpl(context, this);

  osvr::renderkit::GraphicsLibrary myLibrary;
  myLibrary.OpenGL = new osvr::renderkit::GraphicsLibraryOpenGL;
  myLibrary.OpenGL->toolkit = toolkit->getToolkit();

  renderManager = QSharedPointer<osvr::renderkit::RenderManager>(
      osvr::renderkit::createRenderManager(osvrContext->get(), "OpenGL",
                                           myLibrary));
  if (!renderManager || !renderManager->doingOkay())
    throw vx::Exception("de.uni_stuttgart.Voxie.OSVR.Error",
                        "Could not create RenderManager");

  auto ret = renderManager->OpenDisplay();
  if (ret.status == osvr::renderkit::RenderManager::OpenStatus::FAILURE)
    throw vx::Exception("de.uni_stuttgart.Voxie.OSVR.Error",
                        "Could not open display");

  toolkit->makeCurrent(0);

  if (!initializeOpenGLFunctions())
    throw vx::Exception("de.uni_stuttgart.Voxie.OSVR.Error",
                        "Failed to initialize OpenGL functions");

  std::vector<osvr::renderkit::RenderInfo> renderInfo;
  osvrContext->update();
  renderInfo = renderManager->GetRenderInfo();

  glGenFramebuffers(1, &frameBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

  if (!renderManager->RegisterRenderBuffers(colorBuffers)) {
    throw vx::Exception("de.uni_stuttgart.Voxie.OSVR.Error",
                        "RegisterRenderBuffers() returned false");
  }

  for (size_t i = 0; i < renderInfo.size(); i++) {
    GLuint colorBufferName = 0;
    glGenRenderbuffers(1, &colorBufferName);
    osvr::renderkit::RenderBuffer rb;
    rb.OpenGL = new osvr::renderkit::RenderBufferOpenGL;
    rb.OpenGL->colorBufferName = colorBufferName;
    colorBuffers.push_back(rb);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBufferName);

    int width = static_cast<int>(renderInfo[i].viewport.width);
    int height = static_cast<int>(renderInfo[i].viewport.height);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, 0);

    GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    depthBuffers.push_back(depthrenderbuffer);
  }

  // qDebug() << "RegisterRenderBuffers() returned";

  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &OsvrDisplay::updateDisplay);

  timer->start();
}
OsvrDisplay::~OsvrDisplay() {
  for (const auto& dialog : toolkit->getDialogs()) dialog->deleteLater();
}

void OsvrDisplay::updateDisplay() {
  // qDebug() << "timeout";

  toolkit->makeCurrent(0);

  glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

  osvrContext->update();

  auto renderInfos = renderManager->GetRenderInfo();

  for (size_t i = 0; i < renderInfos.size(); i++) {
    auto renderInfo = renderInfos[i];
    auto colorBuffer = colorBuffers[i].OpenGL->colorBufferName;
    auto depthBuffer = depthBuffers[i];

    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    // Set color and depth buffers for the frame buffer
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuffer);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      qCritical() << "RenderView: Incomplete Framebuffer";
      timer->stop();
      deleteLater();
      return;
    }

    // Set the viewport to cover our entire render texture.
    glViewport(0, 0, static_cast<GLsizei>(renderInfo.viewport.width),
               static_cast<GLsizei>(renderInfo.viewport.height));

    // Set to background to red so that it is clear when something went
    // wrong during rendering
    glClearColor(1, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLdouble projection[16];
    osvr::renderkit::OSVR_Projection_to_OpenGL(projection,
                                               renderInfo.projection);
    GLdouble viewM[16];
    osvr::renderkit::OSVR_PoseState_to_OpenGL(viewM, renderInfo.pose);

    GLfloat projectionF[16];
    GLfloat viewF[16];
    for (int i = 0; i < 16; i++) {
      projectionF[i] = projection[i];
      viewF[i] = viewM[i];
    }
    // The QMatrix4x4 constructor accepts row-major order values (unlike
    // the data() member which returns column-major order), so the matrices
    // have to be transposed.
    QMatrix4x4 matProj = QMatrix4x4(projectionF).transposed();
    QMatrix4x4 matView = QMatrix4x4(viewF).transposed();
    render(matProj, matView);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    checkOpenGLStatus();
  }
  if (!renderManager->PresentRenderBuffers(colorBuffers, renderInfos)) {
    // qDebug() << "PresentRenderBuffers() returned false, maybe because it was
    // asked to quit";
    timer->stop();
    deleteLater();
    return;
  }
}
