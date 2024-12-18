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

#include "Visualizer3DView.hpp"

#include <Voxie/Node/Object3DNode.hpp>

#include <Voxie/IVoxie.hpp>

#include <Voxie/MathQt.hpp>

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieBackend/IO/Operation.hpp>

#include <VoxieClient/Array.hpp>

#include <PluginVis3D/Helper/gluhelper.hpp>

#include <PluginVis3D/GeometricPrimitive.hpp>

// #include <PluginVis3D/DebugOptions.hpp>
#include <Voxie/DebugOptions.hpp>

#include <math.h>

#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>
#include <QtGui/QOffscreenSurface>

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QVBoxLayout>

using namespace vx;

class FrameBuffer : protected QOpenGLFunctions {
 public:
  GLuint frameBuffer;
  GLuint colorRenderBuffer;
  GLuint depthRenderBuffer;

  quint32 width;
  quint32 height;

  std::vector<quint32> data;

  QSharedPointer<Object3DPickImageInfo> pickInfo;

  FrameBuffer(quint32 width, quint32 height, GLenum internalformat) {
    this->width = width;
    this->height = height;

    data.resize((size_t)width * height * 4);

    initializeOpenGLFunctions();

    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenRenderbuffers(1, &colorRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, colorRenderBuffer);

    glGenRenderbuffers(1, &depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRenderBuffer);
  }
  ~FrameBuffer() {}

  void bind() { glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer); }
  void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

  void download(const QSharedPointer<Object3DPickImageInfo>& pickInfo,
                GLenum format = GL_RGBA_INTEGER,
                GLenum type = GL_UNSIGNED_INT) {
    this->pickInfo = pickInfo;

    bind();

    memset(data.data(), -1, data.size() * sizeof(quint32));
    // this->glReadBuffer(GL_COLOR_ATTACHMENT0);    // TODO: is this needed?
    // (not in OpenGL ES 2.0)
    this->glReadPixels(0, 0, this->width, this->height, format, type,
                       data.data());

    unbind();

    /*
    uint64_t sum[4] = { 0, 0, 0, 0 };
    for (size_t i = 0; i < data.size(); i++)
      sum[i%4] += data[i];
    qDebug() << "download()" << sum[0] << sum[1] << sum[2] << sum[3];
    */
  }

  std::tuple<quint32, quint32, quint32> lookupRaw(int x, int y) const {
    // qDebug() << "lookupRaw" << x << y << width << height;

    if (x < 0 || (quint32)x >= width || y < 0 || (quint32)y >= height) {
      // This is normal when dragging the mouse and the leaving the window
      // qWarning() << "Attempting to look up position outside pick image";
      return std::make_tuple(0, 0, 0);
    }

    size_t offset = ((size_t)x + (size_t)y * width) * 4;
    return std::make_tuple(data[offset + 0], data[offset + 1],
                           data[offset + 2]);
  }

  std::tuple<Object3DPickImageData*, quint32, quint32> lookupNew(int x,
                                                                 int y) const {
    auto result = lookupRaw(x, y);
    // qDebug() << "lookupNew" << x << y << std::get<0>(result) <<
    // std::get<1>(result) << std::get<2>(result);

    auto datasetId = std::get<0>(result);
    if (!datasetId)
      return std::make_tuple(nullptr, std::get<1>(result), std::get<2>(result));

    if (!pickInfo) {
      qWarning() << "pickInfo is not initialized";
      return std::make_tuple(nullptr, 0, 0);
    }
    auto data = pickInfo->getData(datasetId);
    if (!data) {
      qWarning() << "datasetId out of range";
      return std::make_tuple(nullptr, 0, 0);
    }

    return std::make_tuple(data.data(), std::get<1>(result),
                           std::get<2>(result));
  }

  // TODO: Support having a different resolution for the pick image?
  std::tuple<Object3DPickImageData*, quint32, quint32> lookupNew(
      const vx::Vector<double, 2>& pos) const {
    return lookupNew(std::floor(pos[0]), std::floor(pos[1]));
  }
};

Visualizer3DView::Visualizer3DView(View3DProperties* properties,
                                   vx::visualization::View3D* view3d,
                                   AxisFilter* axisFilter)
    : properties(properties),
      mouseLast{0, 0},
      view3d(view3d),
      axisFilter(axisFilter) {
  this->resize(500 / 96.0 * this->logicalDpiX(),
               400 / 96.0 * this->logicalDpiY());
  setFocusPolicy(Qt::StrongFocus);

  connect(view3d, &vx::visualization::View3D::changed, this,
          [this] { this->update(); });

  connect(properties, &View3DProperties::showViewCenterChanged, this,
          [this] { this->update(); });

  highlightedNode = nullptr;
  haveHighlightedNode = false;
  highlightTimer.setSingleShot(true);
  connect(&highlightTimer, &QTimer::timeout, this, [this]() {
    highlightedNode = nullptr;
    haveHighlightedNode = false;
    update();
  });

  QObject::connect(vx::voxieRoot().activeVisualizerProvider(),
                   &ActiveVisualizerProvider::nodeSelectionChanged, this,
                   [this](const QList<Node*>& selectedNodes) {
                     auto obj =
                         selectedNodes.size() > 0 ? selectedNodes[0] : nullptr;
                     this->highlightNode(obj);
                   });

  updateMouseTracking();

  // Update image when list of objects changed
  connect(properties, &View3DProperties::objectsChanged, this,
          [this](const QList<vx::Node*>& objects) {
            // qDebug() << "Triggering an update because the list of objects
            // changed";
            updateObjects(objects);
            updateMouseTracking();

            updateBoundingBox();

            this->update();
          });
}

Visualizer3DView::~Visualizer3DView() {}

void Visualizer3DView::highlightNode(Node* node) {
  auto hightlightTimeout = 1000;
  // Ignore non-Object3D nodes
  auto object = dynamic_cast<Object3DNode*>(node);
  if (!object) {
    if (haveHighlightedNode) {
      highlightedNode = nullptr;
      haveHighlightedNode = false;
      update();
    }
  } else {
    if (object != highlightedNode) {
      highlightedNode = node;
      haveHighlightedNode = true;
      update();
    }
    highlightTimer.start(hightlightTimeout);
  }
}

// TODO: make sure this is working during destruction of objects
/* Note: When a object is in the list multiple times, only one connection will
 * be created, but that is ok (the connection will be removed when the object is
 * removed the last time). */
void Visualizer3DView::updateObjects(const QList<vx::Node*>& objects) {
  QSet<QDBusObjectPath> found;
  for (const auto& obj : objects) {
    auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
    if (!obj3D) {
      qWarning() << "Object in Objects property is not a Object3D:" << obj;
      continue;
    }
    auto path = obj3D->getPath();
    found.insert(path);
    if (!triggerRenderingConnections.contains(path)) {
      auto connection = QObject::connect(obj3D, &Object3DNode::triggerRendering,
                                         this, [this]() {
                                           /*
                                           qDebug() << "Triggering an update
                                           because" << obj3D
                                                    << "requested it";
                                           */
                                           update();
                                         });
      // qDebug() << "Connect triggerRendering for" << path.path() <<
      // connection;
      triggerRenderingConnections.insert(path, connection);
    }
    if (!needMouseTrackingChangedConnections.contains(path)) {
      auto connection =
          QObject::connect(obj3D, &Object3DNode::needMouseTrackingChanged, this,
                           &Visualizer3DView::updateMouseTracking);
      needMouseTrackingChangedConnections.insert(path, connection);
    }
    if (!boundingBoxChangedConnections.contains(path)) {
      auto connection =
          QObject::connect(obj3D, &Object3DNode::boundingBoxChanged, this,
                           [this]() { updateBoundingBox(); });
      boundingBoxChangedConnections.insert(path, connection);
    }
    if (!setPointCalledConnections.contains(path)) {
      auto connection = QObject::connect(obj3D, &Object3DNode::setPointCalled,
                                         this, &Visualizer3DView::addPoint);
      setPointCalledConnections.insert(path, connection);
    }
  }

  {
    QMap<QDBusObjectPath, QMetaObject::Connection> old =
        triggerRenderingConnections;
    for (const auto& key : old.keys()) {
      if (found.contains(key)) continue;
      qDebug() << "Disconnect triggerRendering for" << key.path() << old[key];
      QObject::disconnect(old[key]);
      triggerRenderingConnections.remove(key);
    }
  }
  {
    QMap<QDBusObjectPath, QMetaObject::Connection> old =
        needMouseTrackingChangedConnections;
    for (const auto& key : old.keys()) {
      if (found.contains(key)) continue;
      qDebug() << "Disconnect needMouseTrackingChanged for" << key.path()
               << old[key];
      QObject::disconnect(old[key]);
      needMouseTrackingChangedConnections.remove(key);
    }
  }
  {
    QMap<QDBusObjectPath, QMetaObject::Connection> old =
        boundingBoxChangedConnections;
    for (const auto& key : old.keys()) {
      if (found.contains(key)) continue;
      qDebug() << "Disconnect boundingBoxChanged for" << key.path() << old[key];
      QObject::disconnect(old[key]);
      boundingBoxChangedConnections.remove(key);
    }
  }
  {
    QMap<QDBusObjectPath, QMetaObject::Connection> old =
        setPointCalledConnections;
    for (const auto& key : old.keys()) {
      if (found.contains(key)) continue;
      qDebug() << "Disconnect setPointCalled for" << key.path() << old[key];
      QObject::disconnect(old[key]);
      setPointCalledConnections.remove(key);
    }
  }
  QMap<QDBusObjectPath, QSharedPointer<Object3DPerShareGroup>>
      oldPerShareGroup = perShareGroup;
  for (const auto& key : oldPerShareGroup.keys()) {
    if (found.contains(key)) continue;
    perShareGroup.remove(key);
  }
}

QString Visualizer3DView::loadShaders() {
  try {
    shaders.reset(
        new Vis3DShaders(this, QSharedPointer<vx::SurfaceData>(),
                         "de.uni_stuttgart.Voxie.ShadingTechnique.Flat"));
  } catch (vx::Exception& e) {
    return e.message();
  }

  return "";
}

QString Visualizer3DView::initialize() {
  QString error = OpenGLDrawWidget::initialize();
  if (error != "") return error;

  QString result = loadShaders();
  if (!result.isEmpty()) return result;

  if (!shaders->renderTriangles().bind()) {
    return "Binding shaders failed";
  }

  pickFrameBuffer.reset(new FrameBuffer(1, 1, GL_RGBA32UI));

  pickFrameBuffer->bind();

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    return "Incomplete pick framebuffer";
  }

  pickFrameBuffer->unbind();

  return "";
}

void Visualizer3DView::updateMouseTracking() {
  bool needMouseTracking = false;
  for (const auto& obj : properties->objects()) {
    try {
      auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
      if (!obj3D) {
        qWarning() << "Object in Objects property is not a Object3D:" << obj;
        continue;
      }
      needMouseTracking |= obj3D->needMouseTracking();
    } catch (Exception& e) {
      qWarning() << "Error while calling needMouseTracking() for object" << obj
                 << ":" << e.message();
    }
  }
  if (vx::debug_option::Log_Vis3D_MouseTracking()->get())
    qDebug() << "setMouseTracking" << needMouseTracking;
  setMouseTracking(needMouseTracking);
}

void Visualizer3DView::setFixedAngle(QString direction) {
  view3d->setFixedAngle(direction);
}

void Visualizer3DView::drawAxisIndicators(const QMatrix4x4& matViewProj) {
  PrimitiveBuffer drawingBuffer;
  QVector3D center = toQVector(vectorCastNarrow<float>(this->view3d->lookAt()));

  auto bb = this->getBoundingBoxWithDefault();
  auto diagonal = std::sqrt(squaredNorm(bb.max() - bb.min()));

  // Draw view center
  if (properties->showViewCenter()) {
    float halfLength = diagonal / 50.0f;

    drawingBuffer.addLine(vx::Color(0.8f, 0.5f, 0.5f),
                          center + QVector3D(-halfLength, 0.0f, 0.0f),
                          center + QVector3D(halfLength, 0.0f, 0.0f));

    drawingBuffer.addLine(vx::Color(0.5f, 0.8f, 0.5f),
                          center + QVector3D(0.0f, -halfLength, 0.0f),
                          center + QVector3D(0.0f, halfLength, 0.0f));

    drawingBuffer.addLine(vx::Color(0.5f, 0.5f, 0.8f),
                          center + QVector3D(0.0f, 0.0f, -halfLength),
                          center + QVector3D(0.0f, 0.0f, halfLength));
  }
  draw(drawingBuffer, matViewProj);
}

void Visualizer3DView::resizeGL(int w, int h) {
  OpenGLDrawWidget::resizeGL(w, h);

  if (!initialized()) return;

  if (this->widthPhysInt() != (int)pickFrameBuffer->width ||
      this->heightPhysInt() != (int)pickFrameBuffer->height)
    // TODO: Support having a different resolution for the pick image?
    pickFrameBuffer.reset(new FrameBuffer(this->widthPhysInt(),
                                          this->heightPhysInt(), GL_RGBA32UI));
}

// TODO: cache bounding box?
vx::BoundingBox3D Visualizer3DView::getBoundingBoxReal() {
  auto boundingBox = BoundingBox3D::empty();
  for (const auto& obj : properties->objects()) {
    try {
      auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
      if (!obj3D) {
        qWarning() << "Object in Objects property is not a Object3D:" << obj;
        continue;
      }
      boundingBox += obj3D->getBoundingBox();
    } catch (Exception& e) {
      qWarning()
          << "Error while getting bounding box / clipping planes for object"
          << obj << ":" << e.message();
    }
  }
  return boundingBox;
}
vx::BoundingBox3D Visualizer3DView::getBoundingBoxWithDefault(
    bool* isDefaultOut) {
  auto boundingBox = this->getBoundingBoxReal();
  if (boundingBox.isEmpty()) {  // Default bounding box
    boundingBox =
        BoundingBox3D::point(vx::Vector<double, 3>(-0.2, -0.2, -0.2)) +
        BoundingBox3D::point(vx::Vector<double, 3>(0.2, 0.2, 0.2));
    if (isDefaultOut) *isDefaultOut = true;
  } else {
    if (isDefaultOut) *isDefaultOut = false;
  }
  return boundingBox;
}

void Visualizer3DView::updateBoundingBox() {
  // qDebug() << "updateBoundingBox()" << zoomInitialized;
  bool isDefault;
  auto boundingBox = getBoundingBoxWithDefault(&isDefault);
  this->getView3D()->setBoundingBox(boundingBox);
  if (!zoomInitialized)
    this->getView3D()->resetView(vx::visualization::View3DProperty::LookAt |
                                 vx::visualization::View3DProperty::ZoomLog);
  zoomInitialized = !isDefault;
}

void Visualizer3DView::renderScreenshot(
    const QSharedPointer<vx::ImageDataPixel>& outputImage,
    const vx::VectorSizeT2& outputRegionStart, const vx::VectorSizeT2& size) {
  quint64 width = size.x;
  quint64 height = size.y;

  if (!outputImage)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Output image is nullptr");
  if (outputRegionStart.x > outputImage->width() ||
      (outputRegionStart.x + width) > outputImage->width() ||
      (outputRegionStart.x + width) < width ||
      outputRegionStart.y > outputImage->height() ||
      (outputRegionStart.y + height) > outputImage->height() ||
      (outputRegionStart.y + height) < height)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Region is outsize image");

  auto viewMatrix = view3d->viewMatrix();
  auto projectionMatrix = view3d->projectionMatrix(width, height);
  auto cameraPosition = view3d->getCameraPosition();

  // qDebug() << "CC X0" << QOpenGLContext::globalShareContext();

  // QOpenGLContext* context = this->context();
  // QSurface* surface = this->context()->surface();

  QOffscreenSurface surfaceObj;
  surfaceObj.create();
  QSurface* surface = &surfaceObj;

  QOpenGLContext contextObj;
  // TODO: Where should this function get its share context from?
  // Using QOpenGLContext::globalShareContext needs Qt::AA_ShareOpenGLContexts
  contextObj.setShareContext(QOpenGLContext::globalShareContext());
  if (!contextObj.create())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Failed to create context");
  QOpenGLContext* context = &contextObj;

  if (!context->makeCurrent(surface))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Failed to make context current");
  if (!QOpenGLContext::currentContext())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "context is nullptr");

  FrameBuffer framebuffer(vx::checked_cast<quint32>(width),
                          vx::checked_cast<quint32>(height), GL_RGBA32F);

  framebuffer.bind();
  glViewport(0, 0, width, height);

  // TODO: cache bounding box / clipping planes?
  auto boundingBox = BoundingBox3D::empty();
  QList<ClippingPlane> clippingPlanes;
  for (const auto& obj : properties->objects()) {
    try {
      auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
      if (!obj3D) {
        qWarning() << "Object in Objects property is not a Object3D:" << obj;
        continue;
      }
      boundingBox += obj3D->getBoundingBox();
      obj3D->getClippingPlanes(clippingPlanes);
    } catch (Exception& e) {
      qWarning()
          << "Error while getting bounding box / clipping planes for object"
          << obj << ":" << e.message();
    }
  }
  if (boundingBox.isEmpty())  // Default bounding box
    boundingBox =
        BoundingBox3D::point(vx::Vector<double, 3>(-0.2, -0.2, -0.2)) +
        BoundingBox3D::point(vx::Vector<double, 3>(0.2, 0.2, 0.2));

  auto pixelSize = view3d->pixelSize(this->size());
  // TODO: use highlightedObject here?
  Object3DRenderContext renderContext(
      context, surface, framebuffer.frameBuffer,
      toQMatrix4x4(matrixCastNarrow<float>(viewMatrix.projectiveMatrix())),
      toQMatrix4x4(
          matrixCastNarrow<float>(projectionMatrix.projectiveMatrix())),
      toQVector(vectorCastNarrow<float>(cameraPosition.hmgVectorData())),
      boundingBox, clippingPlanes, highlightedNode, pixelSize);

  paintImg(renderContext);
  // paintPick(context, surface, framebuffer.frameBuffer, viewMatrix,
  //          projectionMatrix, createQSharedPointer<Object3DPickImageInfo>());
  framebuffer.unbind();

  glViewport(0, 0, this->widthPhys(), this->heightPhys());

  framebuffer.download(QSharedPointer<Object3DPickImageInfo>(), GL_RGBA,
                       GL_FLOAT);
  context->doneCurrent();

  // TODO: Also support 3-component images?
  outputImage->performInGenericContextWithComponents<4>([&](const auto& img) {
    using ComponentType =
        typename std::remove_reference<decltype(*img)>::type::ComponentType;

    const auto& array = img->array();

    for (size_t y = 0; y < size.y; y++) {
      for (size_t x = 0; x < size.x; x++) {
        size_t offset = (x + width * y) * 4;
        auto ptr = (float*)framebuffer.data.data() + offset;
        array(outputRegionStart.x + x, outputRegionStart.y + y) =
            std::array<ComponentType, 4>{
                static_cast<ComponentType>(ptr[0]),
                static_cast<ComponentType>(ptr[1]),
                static_cast<ComponentType>(ptr[2]),
                static_cast<ComponentType>(ptr[3]),
            };
      }
    }
  });
}

// void Visualizer3DView::paint() {
//  paint(view3d->viewMatrix(),
//        view3d->projectionMatrix(this->width(), this->height()));
//}

void Visualizer3DView::paint() {
  glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

  glViewport(0, 0, this->widthPhys(), this->heightPhys());

  auto viewMatrix = view3d->viewMatrix();
  auto projectionMatrix =
      view3d->projectionMatrix(this->widthDIP(), this->heightDIP());
  auto cameraPosition = view3d->getCameraPosition();

  // TODO: cache bounding box / clipping planes?
  auto boundingBox = BoundingBox3D::empty();
  QList<ClippingPlane> clippingPlanes;
  for (const auto& obj : properties->objects()) {
    try {
      auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
      if (!obj3D) {
        qWarning() << "Object in Objects property is not a Object3D:" << obj;
        continue;
      }
      boundingBox += obj3D->getBoundingBox();
      obj3D->getClippingPlanes(clippingPlanes);
    } catch (Exception& e) {
      qWarning()
          << "Error while getting bounding box / clipping planes for object"
          << obj << ":" << e.message();
    }
  }
  if (boundingBox.isEmpty())  // Default bounding box
    boundingBox =
        BoundingBox3D::point(vx::Vector<double, 3>(-0.2, -0.2, -0.2)) +
        BoundingBox3D::point(vx::Vector<double, 3>(0.2, 0.2, 0.2));

  auto pixelSize = view3d->pixelSize(this->size());
  Object3DRenderContext renderContext(
      context(), context()->surface(), defaultFramebufferObject(),
      toQMatrix4x4(matrixCastNarrow<float>(viewMatrix.projectiveMatrix())),
      toQMatrix4x4(
          matrixCastNarrow<float>(projectionMatrix.projectiveMatrix())),
      toQVector(vectorCastNarrow<float>(cameraPosition.hmgVectorData())),
      boundingBox, clippingPlanes, highlightedNode, pixelSize);

  paintImg(renderContext);

  if (!checkOpenGLStatus()) {
    qWarning() << "Error while rendering";
  }

  auto pickInfo = createQSharedPointer<Object3DPickImageInfo>();

  glViewport(0, 0, this->widthPhys(), this->heightPhys());

  Object3DRenderContext renderContextPick(
      context(), context()->surface(), pickFrameBuffer->frameBuffer,
      toQMatrix4x4(matrixCastNarrow<float>(viewMatrix.projectiveMatrix())),
      toQMatrix4x4(
          matrixCastNarrow<float>(projectionMatrix.projectiveMatrix())),
      toQVector(vectorCastNarrow<float>(cameraPosition.hmgVectorData())),
      boundingBox, clippingPlanes, highlightedNode, pixelSize);

  paintPick(renderContextPick,
            pickInfo);  // TODO: do this only when needed

  pickFrameBuffer->unbind();

  if (!checkOpenGLStatus()) {
    qWarning() << "Error while rendering pick image";
  }

  pickFrameBuffer->download(pickInfo);
  if (!checkOpenGLStatus()) {
    qWarning() << "Error while getting pick image";
  }

  /*
  glFinish ();
  pickFrameBuffer->bind ();
  std::vector<quint32> data (this->width() * this->height() * 4);
  memset (data.data (), 0, data.size () * sizeof (quint32));
  glReadBuffer (GL_COLOR_ATTACHMENT0);
  //glReadBuffer (GL_BACK);
  qDebug() << this->width() << this->height();
  if (!checkOpenGLStatus ()) {
      qWarning() << "X1";
  }
  //glReadPixels (0, 0, this->width(), this->height(), GL_DEPTH_COMPONENT,
  GL_FLOAT, data.data ());
  //glReadPixels (0, 0, this->width(), this->height(), GL_RGBA, GL_FLOAT,
  data.data ());
  //glReadPixels (0, 0, this->width(), this->height(), GL_RGBA_INTEGER,
  GL_UNSIGNED_INT, data.data ());
  glReadPixels (0, 0, this->width(), this->height(), GL_RGBA_INTEGER,
  GL_UNSIGNED_INT, data.data ());
  float min = 1.0/0.0, max = -1.0/0.0;
  for (int i = 2; i < data.size (); i += 4) {
      min = std::min (min, (float) data[i]);
      max = std::max (max, (float) data[i]);
  }
  qDebug() << "minmax" << min << max;
  QFile file("/tmp/foo");
  file.open(QIODevice::ReadWrite);
  file.write((const char *)data.data(), data.size() * sizeof(quint32));
  pickFrameBuffer->unbind ();

  if (!checkOpenGLStatus ()) {
      qWarning() << "Error while saving pick image";
  }
  */
}

void Visualizer3DView::paintImg(Object3DRenderContext& renderContext) {
  renderContext.select();

  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  QColor color = this->palette().window().color();

  glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
  glClearDepthf(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    // TODO: cache somewhere?
    auto psgContext = createQSharedPointer<Object3DRenderContextPerShareGroup>(
        renderContext.openGLContext(), renderContext.surface());

    for (const auto& obj : properties->objects()) {
      try {
        auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
        if (!obj3D) {
          qWarning() << "Object in Objects property is not a Object3D:" << obj;
          continue;
        }
        auto path = obj->getPath();
        QSharedPointer<Object3DPerShareGroup> perShareGroupPtr;
        if (perShareGroup.contains(path)) {
          perShareGroupPtr = perShareGroup[path];
        } else {
          renderContext.select();
          perShareGroupPtr = perShareGroup[path] =
              obj3D->newPerShareGroup(psgContext);
        }
        // TODO: cache perContext?
        renderContext.select();
        auto perContext = obj3D->newPerContext(perShareGroupPtr,
                                               renderContext.openGLContext());
        renderContext.select();
        // TODO: set up some stuff like GL_DEPTH_TEST etc. in a predictable way?
        obj3D->render(perShareGroupPtr, perContext, renderContext);
      } catch (Exception& e) {
        qWarning() << "Error while rendering object" << obj << ":"
                   << e.message();
      }
    }
    renderContext.select();
  }

  drawAxisIndicators(renderContext.projectionMatrix() *
                     renderContext.viewMatrix());

  // TODO: code duplication from above
  {
    // TODO: cache somewhere?
    auto psgContext = createQSharedPointer<Object3DRenderContextPerShareGroup>(
        renderContext.openGLContext(), renderContext.surface());

    for (const auto& obj : properties->objects()) {
      try {
        auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
        if (!obj3D) {
          qWarning() << "Object in Objects property is not a Object3D:" << obj;
          continue;
        }
        auto path = obj->getPath();
        QSharedPointer<Object3DPerShareGroup> perShareGroupPtr;
        if (perShareGroup.contains(path)) {
          perShareGroupPtr = perShareGroup[path];
        } else {
          renderContext.select();
          perShareGroupPtr = perShareGroup[path] =
              obj3D->newPerShareGroup(psgContext);
        }
        // TODO: cache perContext?
        renderContext.select();
        auto perContext = obj3D->newPerContext(perShareGroupPtr,
                                               renderContext.openGLContext());
        renderContext.select();
        obj3D->renderTransparent(perShareGroupPtr, perContext, renderContext);
      } catch (Exception& e) {
        qWarning() << "Error while rendering object" << obj << ":"
                   << e.message();
      }
    }
    renderContext.select();
  }

  // While drawing transparent parts, the alpha value in the framebuffer will be
  // changed to some value smaller than 1 (because the alpha value is also
  // blended into the destination buffer using the blend function).
  // When the final framebuffer is not fully opaque, this can cause problems in
  // certain situations (e.g. on Linux with wayland).
  // Reset the alpha values to 1 to avoid this problem.
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Visualizer3DView::paintPick(
    Object3DRenderContext& renderContext,
    const QSharedPointer<Object3DPickImageInfo>& pickInfo) {
  renderContext.select();

  // TODO: Reduce code duplication with paintImg()

  // glViewport(0, 0, this->widthPhys(), this->heightPhys());

  // TODO: this code should probably be in Object3DRenderContext.select()
  glBindFramebuffer(GL_FRAMEBUFFER, renderContext.framebuffer());
  // GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  // glDrawBuffers(1, DrawBuffers);
  // qDebug() << "A" << glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    qCritical() << "paintPick(): Incomplete Framebuffer";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return;
  }

  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  glClearColor(0, 0, 0, 1);
  glClearDepthf(1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    // TODO: cache somewhere?
    auto psgContext = createQSharedPointer<Object3DRenderContextPerShareGroup>(
        renderContext.openGLContext(), renderContext.surface());

    for (const auto& obj : properties->objects()) {
      try {
        auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
        if (!obj3D) {
          qWarning() << "Object in Objects property is not a Object3D:" << obj;
          continue;
        }
        auto path = obj->getPath();
        QSharedPointer<Object3DPerShareGroup> perShareGroupPtr;
        if (perShareGroup.contains(path)) {
          perShareGroupPtr = perShareGroup[path];
        } else {
          renderContext.select();
          perShareGroupPtr = perShareGroup[path] =
              obj3D->newPerShareGroup(psgContext);
        }
        // TODO: cache perContext?
        renderContext.select();
        auto perContext = obj3D->newPerContext(perShareGroupPtr,
                                               renderContext.openGLContext());
        renderContext.select();
        obj3D->renderPick(perShareGroupPtr, perContext, renderContext,
                          *pickInfo);
      } catch (Exception& e) {
        qWarning() << "Error while rendering object" << obj << ":"
                   << e.message();
      }
    }
    renderContext.select();
  }
  glBindFramebuffer(GL_FRAMEBUFFER, renderContext.framebuffer());

  // drawAxisIndicators(projectionMatrix * viewMatrix);

  // TODO: code duplication from above
  // TODO: does a transparent pick image rendering pass make sense?
  {
    // TODO: cache somewhere?
    auto psgContext = createQSharedPointer<Object3DRenderContextPerShareGroup>(
        renderContext.openGLContext(), renderContext.surface());

    for (const auto& obj : properties->objects()) {
      try {
        auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
        if (!obj3D) {
          qWarning() << "Object in Objects property is not a Object3D:" << obj;
          continue;
        }
        auto path = obj->getPath();
        QSharedPointer<Object3DPerShareGroup> perShareGroupPtr;
        if (perShareGroup.contains(path)) {
          perShareGroupPtr = perShareGroup[path];
        } else {
          renderContext.select();
          perShareGroupPtr = perShareGroup[path] =
              obj3D->newPerShareGroup(psgContext);
        }
        // TODO: cache perContext?
        renderContext.select();
        auto perContext = obj3D->newPerContext(perShareGroupPtr,
                                               renderContext.openGLContext());
        renderContext.select();
        obj3D->renderPickTransparent(perShareGroupPtr, perContext,
                                     renderContext, *pickInfo);
      } catch (Exception& e) {
        qWarning() << "Error while rendering object" << obj << ":"
                   << e.message();
      }
    }
    renderContext.select();
  }
}

GeometricPrimitiveNode* Visualizer3DView::findGPO() {
  for (const auto& obj : this->properties->objects()) {
    auto objGeom = dynamic_cast<vx::vis3d::GeometricPrimitive*>(obj);
    if (!objGeom) continue;
    auto gpo = objGeom->getGpo();
    if (gpo) return gpo;
  }
  return nullptr;
}

GeometricPrimitiveNode* Visualizer3DView::findOrAddGPO() {
  auto gpo = findGPO();
  if (gpo) return gpo;
  gpo = dynamic_cast<GeometricPrimitiveNode*>(
      GeometricPrimitiveNode::getPrototypeSingleton()
          ->create(QMap<QString, QVariant>(), QList<Node*>(),
                   QMap<QString, QDBusVariant>())
          .data());
  auto gp = dynamic_cast<vx::vis3d::GeometricPrimitive*>(
      vx::vis3d::GeometricPrimitive::getPrototypeSingleton()
          ->create(QMap<QString, QVariant>(), QList<Node*>(),
                   QMap<QString, QDBusVariant>())
          .data());
  gp->setGpo(gpo);
  auto objects = this->properties->objects();
  objects << gp;
  this->properties->setObjects(objects);
  return gpo;
}

void Visualizer3DView::addPoint(const QVector3D& point) {
  auto gpo = findOrAddGPO();
  auto gpd = gpo->geometricPrimitiveData();
  if (!gpd) {
    gpd = GeometricPrimitiveData::create();
    gpo->setGeometricPrimitiveData(gpd);
  }
  auto primitive = createQSharedPointer<GeometricPrimitivePoint>(
      gpo->nextPointName(), point);
  {
    auto update = gpd->createUpdate();
    gpd->addPrimitive(update, primitive);
    update->finish(QJsonObject());
  }
}

void Visualizer3DView::getTransformedMouseRay(
    const vx::Vector<double, 2>& mousePos, QVector3D translation,
    QQuaternion rotation, QVector3D* start, QVector3D* end) {
  auto viewMatrix = toQMatrix4x4(
      matrixCastNarrow<float>(view3d->viewMatrix().projectiveMatrix()));
  viewMatrix.translate(translation);
  viewMatrix.rotate(rotation);

  // GLU unproject:
  auto fProj = toQMatrix4x4(matrixCastNarrow<float>(
      view3d->projectionMatrix(this->widthDIP(), this->heightDIP())
          .projectiveMatrix()));
  float objNear[4] = {std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::quiet_NaN()};
  float objFar[4] = {std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN()};

  // Calling an OpenGL function would require the correct context
  // GLint viewPort[4];
  // glGetIntegerv(GL_VIEWPORT, viewPort);
  GLint viewPort[4] = {0, 0, (GLint)this->widthDIP(), (GLint)this->heightDIP()};
  if (!gluhelper::glhUnProjectf(mousePos[0], mousePos[1], 0, viewMatrix.data(),
                                fProj.data(), viewPort, objNear)) {
    qWarning() << "glhUnProjectf near failed";
  }
  if (!gluhelper::glhUnProjectf(mousePos[0], mousePos[1], 1, viewMatrix.data(),
                                fProj.data(), viewPort, objFar)) {
    qWarning() << "glhUnProjectf near failed";
  }
  *start = QVector3D(objNear[0], objNear[1], objNear[2]);
  *end = QVector3D(objFar[0], objFar[1], objFar[2]);
}

void Visualizer3DView::mousePressEvent(QMouseEvent* event) {
  auto pos = getMousePosition(this, event);

  view3d->mousePressEvent(mouseLast, pos, event, size());

  // TODO: avoid this stuff if no one uses it?
  // Note: Pick buffer coordinates are physical, not logical coordinates
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  auto pickData = pickFrameBuffer->lookupNew(pos * devicePixelRatioF());
#else
  auto pickData = pickFrameBuffer->lookupNew(pos * devicePixelRatio());
#endif
  QVector3D mouseRayStart, mouseRayEnd;
  getTransformedMouseRay(pos, QVector3D(0, 0, 0), QQuaternion(1, 0, 0, 0),
                         &mouseRayStart, &mouseRayEnd);

  for (const auto& obj : properties->objects()) {
    try {
      auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
      if (!obj3D) {
        qWarning() << "Object in Objects property is not a Object3D:" << obj;
        continue;
      }

      Q_EMIT obj3D->mousePressEvent(event, pickData, mouseRayStart,
                                    mouseRayEnd);
    } catch (Exception& e) {
      qWarning() << "Error while handling mouse press event for object" << obj
                 << ":" << e.message();
    }
  }

  this->mouseLast = pos;
}

void Visualizer3DView::mouseMoveEvent(QMouseEvent* event) {
  auto pos = getMousePosition(this, event);
  // qDebug() << "MME" << pos;

  // TODO: avoid this stuff if no one uses it?
  // Note: Pick buffer coordinates are physical, not logical coordinates
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  auto pickData = pickFrameBuffer->lookupNew(pos * devicePixelRatioF());
#else
  auto pickData = pickFrameBuffer->lookupNew(pos * devicePixelRatio());
#endif
  QVector3D mouseRayStart, mouseRayEnd;
  getTransformedMouseRay(pos, QVector3D(0, 0, 0), QQuaternion(1, 0, 0, 0),
                         &mouseRayStart, &mouseRayEnd);

  for (const auto& obj : properties->objects()) {
    try {
      auto obj3D = dynamic_cast<vx::Object3DNode*>(obj);
      if (!obj3D) {
        qWarning() << "Object in Objects property is not a Object3D:" << obj;
        continue;
      }

      Q_EMIT obj3D->mouseMoveEvent(event, pickData, mouseRayStart, mouseRayEnd);
    } catch (Exception& e) {
      qWarning() << "Error while handling mouse move event for object" << obj
                 << ":" << e.message();
    }
  }

  view3d->mouseMoveEvent(mouseLast, pos, event, size());

  // TODO: Make this depend on modifiers pressed during mousePressEvent
  auto modifiers =
      event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);
  if (event->buttons() == Qt::LeftButton &&
      (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))) {
    auto windowSize = size();

    // TODO: Allow slow move/rotation?
    bool doMove = false;
    double factor = 1.0;
    // if (event->modifiers().testFlag(Qt::AltModifier)) factor = 0.1;
    if (event->modifiers().testFlag(Qt::AltModifier)) doMove = true;

    auto diffUnscaled = pos - mouseLast;

    if (diffUnscaled[0] == 0 && diffUnscaled[1] == 0) return;

    auto diff = diffUnscaled * factor;

    if (!doMove) {
      // TODO: Use arcball vector
      auto matView = view3d->viewMatrix();
      vx::Rotation<double, 3> quatX = vx::rotationFromAxisAngleDeg(
          matView.map(vx::HmgVector<double, 3>({0, 1, 0}, 0)).getVectorPart(),
          diff[0] * 0.15);
      vx::Rotation<double, 3> quatY = vx::rotationFromAxisAngleDeg(
          matView.map(vx::HmgVector<double, 3>({1, 0, 0}, 0)).getVectorPart(),
          -diff[1] * 0.15);
      Q_EMIT this->objectRotationChangeRequested(
          toQQuaternion(rotationCastNarrow<float>(quatX * quatY)));
    } else {
      vx::Vector<double, 3> move(diff[0], diff[1], 0);
      move *= view3d->pixelSize(windowSize);
      auto offset = view3d->orientation().map(move);

      Q_EMIT this->objectPositionChangeRequested(
          toQVector(vectorCastNarrow<float>(offset)));
    }
  }

  this->mouseLast = pos;
}

void Visualizer3DView::mouseReleaseEvent(QMouseEvent* event) {
  auto pos = getMousePosition(this, event);

  view3d->mouseReleaseEvent(mouseLast, pos, event, size());

  this->mouseLast = pos;
}

void Visualizer3DView::wheelEvent(QWheelEvent* event) {
  auto pos = getMousePosition(this, event);
  view3d->wheelEvent(pos, event, size());
}

void Visualizer3DView::keyPressEvent(QKeyEvent* event) {
  view3d->keyPressEvent(event, size());
}
