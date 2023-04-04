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

#include <Voxie/Vis/OpenGLWidget.hpp>
#include <Voxie/Vis/View3D.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <PluginVis3D/Data/AxisFilter.hpp>
#include <PluginVis3D/Data/CuttingPlane.hpp>

#include <PluginVis3D/Prototypes.hpp>

#include <PluginVis3D/Vis3DShaders.hpp>

#include <QMutex>
#include <QtCore/QTimer>

#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLVertexArrayObject>

namespace vx {
class Object3DPerShareGroup;
class GeometricPrimitiveNode;
class ImageDataPixel;
}  // namespace vx

using namespace vx;

class FrameBuffer;  // Defined in isosurfaceview.cpp

/**
 * @brief The Visualizer3DView contains everything related to the 3D view and is
 * responsible for the rendering
 * of the objects, building up the models, handling camera movements and other
 * 3D related tasks.
 */
class Visualizer3DView : public vx::visualization::OpenGLDrawWidget {
  Q_OBJECT

  View3DProperties* properties;

 private:
  QScopedPointer<FrameBuffer> pickFrameBuffer;
  GLuint pickRenderBuffer;
  GLuint pickDepthRenderBuffer;

  // TODO: Create a separate object which takes care of this and which can be
  // destroyed then the object is removed?
  QMap<QDBusObjectPath, QMetaObject::Connection> triggerRenderingConnections;
  QMap<QDBusObjectPath, QMetaObject::Connection>
      needMouseTrackingChangedConnections;
  QMap<QDBusObjectPath, QMetaObject::Connection> boundingBoxChangedConnections;
  QMap<QDBusObjectPath, QMetaObject::Connection> setPointCalledConnections;

  QMap<QDBusObjectPath, QSharedPointer<Object3DPerShareGroup>> perShareGroup;
  void updateObjects(const QList<vx::Node*>& objects);

  bool zoomInitialized = false;

  QPoint mouseLast;

  vx::visualization::View3D* view3d;
  const AxisFilter* axisFilter;

  bool generating = false;

 public:
  Cutting cuttingMode;

 private:
  QPointer<Node> highlightedNode;
  bool haveHighlightedNode;  // This is needed because highlightedNode might
                             // be set to nullptr when the node is destroyed
  QTimer highlightTimer;
  void highlightNode(Node* node);

  QScopedPointer<Vis3DShaders> shaders;

  void uploadData();

  vx::BoundingBox3D getBoundingBoxReal();
  vx::BoundingBox3D getBoundingBoxWithDefault(bool* isDefaultOut = nullptr);
  void updateBoundingBox();

  /**
   * @brief drawAxisIndicators Draws multiple indicator for orientation in the
   * scene, mostly lines, like the bound boxes,
   * axis lines, view center point etc.
   * @param projectionMatrix
   */
  void drawAxisIndicators(const QMatrix4x4& projectionMatrix);

  GeometricPrimitiveNode* findGPO();
  GeometricPrimitiveNode* findOrAddGPO();

  void addPoint(const QVector3D& point);

  /**
   * @brief getTransformedMouseRay Calculates a mouse ray starting from the
   * camera position and going straight to a far point the camera is looking at.
   * An additional translation and rotation can be applied to adapt the mouse
   * ray's transform to a specific object.
   * The translation is needed because it is easier transform the mouse ray than
   * transforming the hit object according to it's transform in the world.
   * @param event
   * @param translation A translation to apply to the mouse ray. This is for
   * example the rotation and translation of a surface
   * @param rotation A rotation to apply to the mouse ray. This is for example
   * the rotation and translation of an 3D object.
   * @param start Contains the 3D position of the mouse ray's start point. This
   * is the point on the near plane in the view space.
   * @param end Contains the 3D Position of the mouse ray's end point. This is
   * the point on the far plane in the view space.
   */
  void getTransformedMouseRay(QMouseEvent* event, QVector3D translation,
                              QQuaternion rotation, QVector3D* start,
                              QVector3D* end);

  /**
   * @brief loadShaders Compiles and loads the vertex and fragment shader.
   * @return
   */
  QString loadShaders();

 public:
  explicit Visualizer3DView(
      View3DProperties* properties, vx::visualization::View3D* view3,
      AxisFilter* axisFilter);

  ~Visualizer3DView();

  /**
   * Check whether any object needs mouse tracking and call setMouseTracking().
   */
  void updateMouseTracking();

  virtual QString initialize() override;
  virtual void resizeGL(int w, int h) override;
  virtual void paint() override;

  // void paint(const QMatrix4x4& viewMatrix, const QMatrix4x4&
  // projectionMatrix);
  void renderScreenshot(const QSharedPointer<vx::ImageDataPixel>& outputImage,
                        const vx::VectorSizeT2& outputRegionStart,
                        const vx::VectorSizeT2& size);

  void paintImg(Object3DRenderContext& renderContext);
  void paintPick(Object3DRenderContext& renderContext,
                 const QSharedPointer<Object3DPickImageInfo>& pickInfo);

  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

  /** @brief setFixedAngle Sets a fixed viewing angle
   * @param direction The new view direction
   */
  void setFixedAngle(QString direction);

  vx::visualization::View3D* getView3D() const { return view3d; }

  /**
   * @brief setCuttingMode Sets the cutting mode and cutting limit for the
   given surface.
   * @param Surface
   * @param mode
   * @param limit
   */
  void setCuttingMode(CuttingMode mode, int limit = -1) {
    cuttingMode.setMode(mode, limit);
  }

 Q_SIGNALS:
  void projectionChanged();

  /**
   * @brief cuttingModeChanged is signaled if the cutting mode of a dataset is
   * changed.
   */
  void cuttingModeChanged();

  /**
   * @brief objectPositionChangeRequested is signalled if a other class request
   * the object position.
   * @param QVector3D offset
   */
  void objectPositionChangeRequested(QVector3D offset);

  /**
   * @brief objectRotationChangeRequested is signalled if a other class request
   * the object rotation.
   * @param QQuaternion angle
   */
  void objectRotationChangeRequested(QQuaternion angle);
};
