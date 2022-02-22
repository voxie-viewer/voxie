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

#include "Object3DNode.hpp"

#include <Voxie/Node/NodePrototype.hpp>

#include <QtGui/QOpenGLFunctions>

#include <limits>

using namespace vx;

Object3DRenderContextPerShareGroup::Object3DRenderContextPerShareGroup(
    QOpenGLContext* openGLShareContext, QSurface* shareSurface)
    : openGLShareContext_(openGLShareContext), shareSurface_(shareSurface) {
  if (!openGLShareContext)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Object3DRenderContextPerShareGroup::"
                        "Object3DRenderContextPerShareGroup("
                        "): openGLShareContext is nullptr");
  if (!shareSurface)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Object3DRenderContextPerShareGroup::"
                        "Object3DRenderContextPerShareGroup("
                        "): shareSurface is nullptr");
}
Object3DRenderContextPerShareGroup::~Object3DRenderContextPerShareGroup() {}

void Object3DRenderContextPerShareGroup::select() {
  if (!this->openGLShareContext()->makeCurrent(this->shareSurface()))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Failed to make OpenGL share context current");
}
void Object3DRenderContextPerShareGroup::unselect() {
  openGLShareContext()->doneCurrent();
}

Object3DRenderContext::Object3DRenderContext(
    QOpenGLContext* openGLContext, QSurface* surface, GLuint framebuffer,
    const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix,
    const QVector4D& lightPosition, const BoundingBox3D& boundingBox,
    const QList<ClippingPlane>& clippingPlanes, Node* highlightedNode,
    double pixelSize)
    : openGLContext_(openGLContext),
      surface_(surface),
      framebuffer_(framebuffer),
      viewMatrix_(viewMatrix),
      projectionMatrix_(projectionMatrix),
      lightPosition_(lightPosition),
      boundingBox_(boundingBox),
      clippingPlanes_(clippingPlanes),
      highlightedNode_(highlightedNode),
      pixelSize_(pixelSize) {
  if (!openGLContext)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Object3DRenderContext::Object3DRenderContext("
                        "): openGLContext is nullptr");
  if (!surface)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Object3DRenderContext::Object3DRenderContext("
                        "): surface is nullptr");

  if (!this->openGLContext()->makeCurrent(this->surface()))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Failed to make OpenGL context current");
  functions = createQSharedPointer<QOpenGLFunctions>();
  functions->initializeOpenGLFunctions();
  unselect();
}
Object3DRenderContext::~Object3DRenderContext() {}

void Object3DRenderContext::select() {
  if (!this->openGLContext()->makeCurrent(this->surface()))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Failed to make OpenGL context current");
  functions->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer());
}

void Object3DRenderContext::unselect() {
  if (QOpenGLContext::currentContext() != openGLContext()) return;
  functions->glBindFramebuffer(GL_FRAMEBUFFER, 0);
  openGLContext()->doneCurrent();
}

Object3DPickImageData::Object3DPickImageData(Object3DNode* object)
    : object_(object) {}
Object3DPickImageData::~Object3DPickImageData() {}

Object3DPickImageInfo::Object3DPickImageInfo() {}
Object3DPickImageInfo::~Object3DPickImageInfo() {}

QSharedPointer<Object3DPickImageData> Object3DPickImageInfo::getData(
    uint32_t id) {
  // TODO: allow multi-threading
  if (QThread::currentThread() != QCoreApplication::instance()->thread())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Object3DPickImageInfo::getData() called from non-main thread");
  if (!info().contains(id))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid data ID");
  return info()[id];
}

uint32_t Object3DPickImageInfo::addData(
    const QSharedPointer<Object3DPickImageData>& data) {
  // TODO: allow multi-threading
  if (QThread::currentThread() != QCoreApplication::instance()->thread())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Object3DPickImageInfo::getData() called from non-main thread");
  if (lastId_ == std::numeric_limits<uint32_t>::max())
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Pick ID overflow");
  lastId_++;
  auto id = lastId_;
  info_[id] = data;
  return id;
}

Object3DPerShareGroup::Object3DPerShareGroup() {}
Object3DPerShareGroup::~Object3DPerShareGroup() {}

Object3DPerContext::Object3DPerContext() {}
Object3DPerContext::~Object3DPerContext() {}

Object3DNode::Object3DNode(const QSharedPointer<NodePrototype>& prototype)
    : Node("Object3DNode", prototype) {
  setAutomaticDisplayName(prototype->displayName());

  // TODO: should this be here?
  QObject::connect(this, &Object3DNode::clippingPlanesChanged, this,
                   &Object3DNode::triggerRendering);
}
Object3DNode::~Object3DNode() {}

QList<QString> Object3DNode::supportedDBusInterfaces() { return {}; }

QSharedPointer<Object3DPerShareGroup> Object3DNode::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareContext) {
  Q_UNUSED(shareContext);
  return QSharedPointer<Object3DPerShareGroup>();
}
QSharedPointer<Object3DPerContext> Object3DNode::newPerContext(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    QOpenGLContext* context) {
  Q_UNUSED(perShareGroup);
  Q_UNUSED(context);
  return QSharedPointer<Object3DPerContext>();
}

void Object3DNode::renderPick(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context, Object3DPickImageInfo& pickInfo) {
  Q_UNUSED(perShareGroup);
  Q_UNUSED(perContext);
  Q_UNUSED(context);
  Q_UNUSED(pickInfo);
  // Default is to do nothing, object is invisible on pick image
}

void Object3DNode::renderTransparent(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context) {
  Q_UNUSED(perShareGroup);
  Q_UNUSED(perContext);
  Q_UNUSED(context);
  // Default is to do no transparent rendering
}

void Object3DNode::renderPickTransparent(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context, Object3DPickImageInfo& pickInfo) {
  Q_UNUSED(perShareGroup);
  Q_UNUSED(perContext);
  Q_UNUSED(context);
  Q_UNUSED(pickInfo);
  // Default is to do no transparent rendering
}

void Object3DNode::getClippingPlanes(QList<ClippingPlane>& planes) {
  Q_UNUSED(planes);
}

bool Object3DNode::needMouseTracking() { return false; }

bool Object3DNode::isAllowedChild(NodeKind kind) {
  return kind == NodeKind::Visualizer;
}
bool Object3DNode::isAllowedParent(NodeKind kind) {
  return kind == NodeKind::Data || kind == NodeKind::Property;
}
