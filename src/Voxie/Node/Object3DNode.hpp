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

#include <Voxie/Data/BoundingBox3D.hpp>

#include <Voxie/Node/Node.hpp>

#include <QtCore/QPointer>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>
#include <QtGui/QOpenGLContext>

// TODO: should there be an object for a Object3D being displayed in a
// particular view?

namespace vx {
class Object3DNode;

class VOXIECORESHARED_EXPORT ClippingPlane {
  QVector4D planeEquation_;

 public:
  ClippingPlane(const QVector4D& planeEquation)
      : planeEquation_(planeEquation) {}

  /**
   * The plane equation in the form accepted by glClipPlane().
   */
  const QVector4D& planeEquation() const { return planeEquation_; }
};

class VOXIECORESHARED_EXPORT Object3DRenderContextPerShareGroup {
  QOpenGLContext* openGLShareContext_;
  QSurface* shareSurface_;

 public:
  Object3DRenderContextPerShareGroup(QOpenGLContext* openGLShareContext,
                                     QSurface* shareSurface);
  ~Object3DRenderContextPerShareGroup();

  QOpenGLContext* openGLShareContext() const { return openGLShareContext_; }

  // Returns a surface which can be used to make the share context current
  QSurface* shareSurface() const { return shareSurface_; }

  /**
   * Make an OpenGL context in the correct share group current.
   */
  void select();

  void unselect();
};

class VOXIECORESHARED_EXPORT Object3DRenderContext {
  QOpenGLContext* openGLContext_;
  QSharedPointer<QOpenGLFunctions> functions;
  QSurface* surface_;
  GLuint framebuffer_;
  QMatrix4x4 viewMatrix_;
  QMatrix4x4 projectionMatrix_;
  QVector4D lightPosition_;
  BoundingBox3D boundingBox_;
  QList<ClippingPlane> clippingPlanes_;
  Node* highlightedNode_;
  double pixelSize_;

 public:
  Object3DRenderContext(QOpenGLContext* openGLContext, QSurface* surface,
                        GLuint framebuffer, const QMatrix4x4& viewMatrix,
                        const QMatrix4x4& projectionMatrix,
                        const QVector4D& lightPosition,
                        const BoundingBox3D& boundingBox,
                        const QList<ClippingPlane>& clippingPlanes,
                        Node* highlightedNode, double pixelSize);
  virtual ~Object3DRenderContext();

  QOpenGLContext* openGLContext() const { return openGLContext_; }
  QSurface* surface() const { return surface_; }
  GLuint framebuffer() const { return framebuffer_; }
  const QMatrix4x4& viewMatrix() const { return viewMatrix_; }
  const QMatrix4x4& projectionMatrix() const { return projectionMatrix_; }
  const QVector4D& lightPosition() const { return lightPosition_; }
  const BoundingBox3D& boundingBox() const { return boundingBox_; }
  const QList<ClippingPlane>& clippingPlanes() const { return clippingPlanes_; }
  Node* highlightedNode() const { return highlightedNode_; }
  double pixelSize() const { return pixelSize_; }

  /**
   * Make the OpenGL context current, select the framebuffer etc.
   */
  void select();

  void unselect();
};

/**
 * One instance of this class will be generated whenever a 3D object is rendered
 * in a new OpenGL share group. The Object3D implementation can store
 * information in this instance and use it during rendering.
 */
class VOXIECORESHARED_EXPORT Object3DPerShareGroup : public QObject {
  Q_OBJECT

 public:
  Object3DPerShareGroup();
  virtual ~Object3DPerShareGroup();
};

/**
 * One instance of this class will be generated whenever a 3D object is rendered
 * in a new OpenGL context. The Object3D implementation can store information in
 * this instance and use it during rendering.
 */
class VOXIECORESHARED_EXPORT Object3DPerContext : public QObject {
  Q_OBJECT

 public:
  Object3DPerContext();
  virtual ~Object3DPerContext();
};

class VOXIECORESHARED_EXPORT Object3DPickImageData {
  QPointer<Object3DNode> object_;

 public:
  Object3DPickImageData(Object3DNode* object);
  virtual ~Object3DPickImageData();

  Object3DNode* object() const { return object_; }
};

// TODO: should this be safe for multithreading?
class VOXIECORESHARED_EXPORT Object3DPickImageInfo {
  uint32_t lastId_;
  QMap<uint32_t, QSharedPointer<Object3DPickImageData>> info_;

 public:
  Object3DPickImageInfo();
  virtual ~Object3DPickImageInfo();

  const QMap<uint32_t, QSharedPointer<Object3DPickImageData>>& info() const {
    return info_;
  }

  QSharedPointer<Object3DPickImageData> getData(uint32_t id);

  // The returned ID must be used in the red channel of the pick image
  uint32_t addData(const QSharedPointer<Object3DPickImageData>& data);
};

class VOXIECORESHARED_EXPORT Object3DNode : public Node {
  Q_OBJECT

 public:
  Object3DNode(const QSharedPointer<NodePrototype>& prototype);
  ~Object3DNode();

  QList<QString> supportedDBusInterfaces() override;

  /**
   * Will be called whenever a new OpenGL share group has been used for
   * rendering an object. The returned instance will be cached and passed to
   * rendering functions. The function can also return a nullptr if it doesn't
   * need any per-share-group information.
   *
   * The passed shareContext parameter contains an OpenGL context which shares
   * with the context which is used for rendering later on (or is the same).
   *
   * The OpenGL context will already have been made current.
   *
   * The default implementation returns nullptr.
   *
   * This method may throw a vx::Exception.
   */
  virtual QSharedPointer<Object3DPerShareGroup> newPerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareContext);

  /**
   * Will be called whenever a new OpenGL context has been used for rendering an
   * object. The returned instance will be cached and passed to rendering
   * functions. The function can also return a nullptr if it doesn't need any
   * per-context information.
   *
   * The passed perShareGroup information is the information which was returned
   * by newPerShareGroup().
   *
   * The passed context parameter is the OpenGL context which will be used for
   * rendering.
   *
   * The OpenGL context will already have been made current.
   *
   * The default implementation returns nullptr.
   *
   * This method may throw a vx::Exception.
   */
  virtual QSharedPointer<Object3DPerContext> newPerContext(
      const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
      QOpenGLContext* context);

  /**
   * Will be called when the object should be rendered. The parameters for
   * rendering are available in context.
   *
   * The OpenGL context will already have been made current, the framebuffer /
   * viewport will be set etc.
   *
   * This method may throw a vx::Exception.
   */
  virtual void render(
      const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
      const QSharedPointer<Object3DPerContext>& perContext,
      Object3DRenderContext& context) = 0;

  /**
   * Will be called when the object should be rendered into the pick image.
   * The parameters for rendering are available in context.
   *
   * The OpenGL context will already have been made current, the framebuffer /
   * viewport will be set etc.
   *
   * This method may throw a vx::Exception.
   */
  virtual void renderPick(
      const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
      const QSharedPointer<Object3DPerContext>& perContext,
      Object3DRenderContext& context, Object3DPickImageInfo& pickInfo);

  virtual void renderTransparent(
      const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
      const QSharedPointer<Object3DPerContext>& perContext,
      Object3DRenderContext& context);

  // TODO: Should this be removed? (Transparent objects in the pick image don't
  // really make sense)
  virtual void renderPickTransparent(
      const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
      const QSharedPointer<Object3DPerContext>& perContext,
      Object3DRenderContext& context, Object3DPickImageInfo& pickInfo);

  /**
   * This function should update min/max to include the area where the 3D object
   * wants to display data. The object can also display things outside this box,
   * the box is used e.g. for zooming etc.
   */
  virtual BoundingBox3D getBoundingBox() = 0;

  /**
   * This function should add all clipping planes defined by this object to
   * the list planes.
   *
   * The default implementation adds no planes.
   */
  virtual void getClippingPlanes(QList<ClippingPlane>& planes);

  /**
   * This function should return true if the mouseMoveEvents should be emitted
   * even if the mouse button is not pressed.
   */
  virtual bool needMouseTracking();

  // TODO: remove?
  bool isAllowedChild(NodeKind kind) override;
  bool isAllowedParent(NodeKind kind) override;

 Q_SIGNALS:
  /**
   * This signal should be emitted whenever the object changes and requires the
   * scene to be rerendered.
   */
  void triggerRendering();

  /**
   * This signal should be emitted whenever the bounding box has changed.
   */
  void boundingBoxChanged();

  /**
   * This signal should be emitted whenever the the clipping planes have
   * changed.
   */
  void clippingPlanesChanged();

  /**
   * This signal should be emitted whenever the the needMouseTracking property
   * changes.
   */
  void needMouseTrackingChanged(bool needMouseTracking);

  // TODO: move?
  void setPointCalled(const QVector3D& position);

  // TODO: move?
  void objectPositionChangeRequested(const QVector3D& offset);
  void objectRotationChangeRequested(const QQuaternion& rotation);

  // TODO: should this be here? Should be cleaned up
  void mouseMoveEvent(
      QMouseEvent* event,
      std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
      const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd);
  void mousePressEvent(
      QMouseEvent* event,
      std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
      const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd);
};
}  // namespace vx
