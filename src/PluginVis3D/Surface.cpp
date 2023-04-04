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

#include "Surface.hpp"

#include <VoxieBackend/Data/SurfaceAttribute.hpp>

#include <Voxie/IVoxie.hpp>

#include <Voxie/Data/SurfaceNode.hpp>

#include <Voxie/Node/PropertyHelper.hpp>

#include <Voxie/Vis/OpenGLWidget.hpp>

#include <PluginVis3D/Prototypes.hpp>
#include <PluginVis3D/Visualizer3DView.hpp>  // TODO: remove

#include <PluginVis3D/Data/SurfaceData3D.hpp>

#include <QtGui/QOpenGLFunctions>

// TODO: Move most stuff out of SurfacePerShareGroup/SurfaceData into Surface
// class, keep only stuff which really needs the OpenGL context into
// SurfacePerShareGroup // TODO: Create a SurfaceImpl (?) class which would be
// created when a Surface is connected and would be destroyed when the surface
// connection is changed? (same for plane.cpp)

// TODO: Replace qCritical() by exceptions?

// TODO: Changes to the surface a currently not picked up (no redraw is
// triggered, cached surface do not seem to be updated) (at least when the
// object graph is loaded from a file).

using namespace vx;
using namespace vx::visualization;
using namespace vx::vis3d;

namespace {
class SurfacePerShareGroup : public Object3DPerShareGroup {
 public:
  QSharedPointer<Object3DRenderContextPerShareGroup> shareGroup;

  QPointer<vx::vis3d::Surface> surfaceObj;

  QOpenGLFunctions functions;

  QSharedPointer<SurfaceData3D> data;

  QScopedPointer<Vis3DShaders> shaders;

  QSharedPointer<OpenGLDrawUtils> drawUtils;

  SurfacePerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup)
      : shareGroup(shareGroup) {
    functions.initializeOpenGLFunctions();

    drawUtils.reset(new OpenGLDrawUtils);
    drawUtils->initialize();

    // Initialize temporary shaders without attributes
    shaders.reset(
        new Vis3DShaders(&functions, QSharedPointer<vx::SurfaceData>(),
                         "de.uni_stuttgart.Voxie.ShadingTechnique.Flat"));
  }

  // Must be called with OpenGL context current
  void setSurfaceData(vx::vis3d::Surface* surfaceObj,
                      vx::SurfaceNode* surface) {
    this->surfaceObj = surfaceObj;

    QSharedPointer<QOpenGLBuffer> indexBuffer(
        new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer));
    if (!indexBuffer->create())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Surface index buffer create failed");

    QSharedPointer<QOpenGLBuffer> vertexBuffer(new QOpenGLBuffer());
    if (!vertexBuffer->create())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Surface vertex buffer create failed");

    QSharedPointer<QOpenGLBuffer> normalBuffer(new QOpenGLBuffer());
    if (!normalBuffer->create())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Surface normal buffer create failed");

    // TODO: This is a hack, what kind of pointer should be used here?
    auto surfaceShared = QSharedPointer<vx::SurfaceNode>(
        surface, [](vx::SurfaceNode* ptr) { Q_UNUSED(ptr); });

    QSharedPointer<SurfaceData3D> surfaceData(
        new SurfaceData3D(surfaceShared, indexBuffer, vertexBuffer,
                          normalBuffer),
        [](QObject* obj) { obj->deleteLater(); });

    if (!surfaceData->getSurface()) {
      // This happens e.g. when there is an empty data node connected
      // qWarning() << "Not a SurfaceDataTriangleIndexed, ignoring";
      return;
    }

    shaders.reset(new Vis3DShaders(&functions, surfaceData->getSurface(),
                                   surfaceObj->shadingTechnique()));

    auto& surfaceAttributeBuffers = surfaceData->attributeBuffer();
    for (auto attribute : surfaceData->getSurface()->listAttributes()) {
      if (attribute->kind() !=
              "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex" &&
          attribute->kind() !=
              "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle") {
        qWarning() << "Unknown attribute kind in Surface.cpp:"
                   << attribute->kind();
        continue;
      }

      auto name = attribute->name();
      QString nameEscaped = name;
      nameEscaped.replace('.', '_');

      // only create a buffer if the data is actually used by the shader
      auto attributeLocation =
          functions.glGetAttribLocation(shaders->renderTriangles().programId(),
                                        nameEscaped.toStdString().c_str());
      if (attributeLocation == -1) {
        continue;
      }

      surfaceAttributeBuffers[name] =
          QSharedPointer<QOpenGLBuffer>(new QOpenGLBuffer());

      if (!surfaceAttributeBuffers[name]->create())
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "SurfaceAttribute buffer create failed");
    }

    data = surfaceData;
    uploadData();
  }

  // Like QVector3D::normalized(), but with a smaller minimum size
  static QVector3D normalized(const QVector3D& value) {
    double absSquared = double(value.x()) * double(value.x()) +
                        double(value.y()) * double(value.y()) +
                        double(value.z()) * double(value.z());
    if (absSquared >= 1e-50) {  // minimum length 1e-25
      double len = std::sqrt(absSquared);
      return QVector3D(float(double(value.x()) / len),
                       float(double(value.y()) / len),
                       float(double(value.z()) / len));
    } else {
      return QVector3D();
    }
  }

  // Must be called with OpenGL context current
  void uploadData() {
    shareGroup->select();

    // qDebug() << "uploadData()";

    auto surface = this->surfaceObj;
    if (!surface) {
      qWarning() << "Surface object has already been destroyed or is not set";
      return;
    }

    auto surfaceData = data;
    auto surfaceModified = surfaceData->createSurfaceModified();

    std::vector<QVector3D> normals(surfaceModified->vertices().size());
    for (size_t i = 0; i < surfaceModified->triangles().size(); i++) {
      const auto& triangle = surfaceModified->triangles()[i];
      QVector3D a = surfaceModified->vertices()[triangle[0]];
      QVector3D b = surfaceModified->vertices()[triangle[1]];
      QVector3D c = surfaceModified->vertices()[triangle[2]];

      QVector3D normal = normalized(QVector3D::crossProduct(b - c, c - a));

      // TODO: What to do with very small triangles where the cross product is
      // almost zero?
      /*
        if (normal == QVector3D(0,0,0))
        qDebug()<<"X"<<triangles[i].p[0]<<triangles[i].p[1]<<triangles[i].p[2];
      */

      // copy the normal of the triangle to the last vertex for flat shading
      normals[triangle[2]] = normal;
    }

    auto indexBuffer = surfaceData->getIndexBuffer();
    auto vertexBuffer = surfaceData->getVertexBuffer();
    auto normalBuffer = surfaceData->getNormalBuffer();

    if (!indexBuffer->bind())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Binding index buffer failed");
    static_assert(
        sizeof(uint32_t) == sizeof(vx::SurfaceDataTriangleIndexed::IndexType),
        "Index types don't match");
    functions.glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        surfaceModified->triangles().size() * 3 * sizeof(uint32_t),
        surfaceModified->triangles().data(), GL_STATIC_DRAW);

    if (!vertexBuffer->bind())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Binding vertex buffer failed");
    // functions.glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
    // vertices.data(), GL_STATIC_DRAW);
    functions.glBufferData(
        GL_ARRAY_BUFFER,
        surfaceModified->vertices().size() * 3 * sizeof(float_t),
        surfaceModified->vertices().data(), GL_STATIC_DRAW);

    if (!normalBuffer->bind())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Binding surface normal buffer failed");
    functions.glBufferData(GL_ARRAY_BUFFER, normals.size() * 3 * sizeof(float),
                           normals.data(), GL_STATIC_DRAW);

    // bind SurfaceAttribute attributes
    auto& surfaceAttributeBuffer = surfaceData->attributeBuffer();

    for (auto entry : surfaceAttributeBuffer) {
      auto name = entry.first;
      auto buffer = entry.second;

      if (!buffer->bind())
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Binding SurfaceAttribute buffer failed");

      auto attribute = surfaceModified->getAttribute(name);
      functions.glBufferData(GL_ARRAY_BUFFER, attribute->getByteSize(),
                             attribute->getBytes(), GL_STATIC_DRAW);
    }

    // auto numVertices = vertices.size() / 3;
    //(*dataSet)->setVertexInfo(numVertices);
    // vertexCount += numVertices;

    if (this->surfaceObj)
      this->surfaceObj
          ->triggerRendering();  // TODO: should this really be called here?
  }

  int lastClipAmount = 0;
  void disableClipDistances(int start) {
    for (int i = start; i < lastClipAmount; i++) {
      functions.glDisable(GL_CLIP_DISTANCE0 + i);
    }
  }

  void setupCuttingPlanes(const QList<ClippingPlane>& clippingPlanes) {
    if (!shaders) {
      qCritical() << "Attempting to call setupCuttingPlanes() without shaders";
      return;
    }

    int numClipPlanes = clippingPlanes.size();

    if (numClipPlanes > 0) {
      if (numClipPlanes > shaders->maxClipDistances()) {
        qWarning() << "Warning: Maximum number of "
                   << shaders->maxClipDistances()
                   << " cutting planes exceeded. "
                   << (numClipPlanes - shaders->maxClipDistances())
                   << " cutting planes will be neglected.";
        numClipPlanes = shaders->maxClipDistances();
      }
      GLfloat* equations = new GLfloat[numClipPlanes * 4];
      GLint* cuttingDirections = new GLint[numClipPlanes];
      int index = 0;
      for (const auto& plane : clippingPlanes) {
        if (index >= numClipPlanes) break;

        // Always use positive, the plane equation will already be negated if
        // needed
        cuttingDirections[index] = CuttingDirection::Positive;
        functions.glEnable(GL_CLIP_DISTANCE0 + index);
        equations[index * 4 + 0] = plane.planeEquation().x();
        equations[index * 4 + 1] = plane.planeEquation().y();
        equations[index * 4 + 2] = plane.planeEquation().z();
        equations[index * 4 + 3] = plane.planeEquation().w();

        index++;
      }
      // TODO: Get from RenderContext?
      // auto cuttingMode = getAndSetCuttingMode(this->selectedSurface());
      // auto cuttingModeMode = cuttingMode->mode();
      // auto cuttingModeLimit = cuttingMode->limit();
      auto cuttingModeMode = AtLeast;
      auto cuttingModeLimit = 1;

      // Send data to shader
      functions.glUniform1i(shaders->renderTriangles_cuttingMode(),
                            cuttingModeMode);
      functions.glUniform1i(shaders->renderTriangles_cuttingLimit(),
                            cuttingModeLimit);
      functions.glUniform1iv(shaders->renderTriangles_clippingDirections(),
                             numClipPlanes, cuttingDirections);
      functions.glUniform4fv(shaders->renderTriangles_clippingPlanes(),
                             numClipPlanes, equations);
      delete[] equations;
      delete[] cuttingDirections;
    } else {
      functions.glDisable(GL_CLIP_DISTANCE0);
    }
    // Send number of planes to shader
    functions.glUniform1i(shaders->renderTriangles_numClipDistances(),
                          numClipPlanes);

    // Disable all other clip distances that were previously enabled:
    disableClipDistances(numClipPlanes);
    lastClipAmount = numClipPlanes;
  }
};

class SurfacePickData : public Object3DPickImageData {
 public:
  SurfacePickData(Object3DNode* object) : Object3DPickImageData(object) {}
  ~SurfacePickData() {}

  QSharedPointer<SurfaceData3D> data;
};
}  // namespace

Surface::Surface()
    : Object3DNode(getPrototypeSingleton()),
      properties(new object3d_prop::SurfaceProperties(this)) {
  QObject::connect(this, &Surface::mouseMoveEvent, this,
                   &Surface::mouseMoveEventImpl);
  QObject::connect(this, &Surface::mousePressEvent, this,
                   &Surface::mousePressEventImpl);

  QObject::connect(this, &Object3DNode::objectPositionChangeRequested, this,
                   [this](const QVector3D& offset) {
                     auto surfaceObj =
                         dynamic_cast<SurfaceNode*>(properties->surface());
                     if (!surfaceObj) return;
                     surfaceObj->adjustPosition(offset);
                   });
  QObject::connect(this, &Object3DNode::objectRotationChangeRequested, this,
                   [this](const QQuaternion& rotation) {
                     auto surfaceObj =
                         dynamic_cast<SurfaceNode*>(properties->surface());
                     if (!surfaceObj) return;
                     surfaceObj->adjustRotation(rotation);
                   });

  // Reevaluate needMouseTracking() and rerender when highlightCurrentTriangle
  // changes
  QObject::connect(
      properties,
      &object3d_prop::SurfaceProperties::highlightCurrentTriangleChanged, this,
      &Surface::needMouseTrackingChanged);
  QObject::connect(
      properties,
      &object3d_prop::SurfaceProperties::highlightCurrentTriangleChanged, this,
      &Surface::triggerRendering);

  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::surfaceChanged, this,
                   &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::faceCullingChanged, this,
                   &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::visibleChanged, this,
                   &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::drawBoundingBoxChanged,
                   this, &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::drawOriginChanged, this,
                   &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::drawAxisArrowsChanged,
                   this, &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::frontColorChanged, this,
                   &Surface::triggerRendering);
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::backColorChanged, this,
                   &Surface::triggerRendering);

  // Update bounding box when surface changes
  QObject::connect(properties,
                   &object3d_prop::SurfaceProperties::surfaceChanged, this,
                   &Surface::boundingBoxChanged);

  forwardSignalFromPropertyNode(
      properties, &object3d_prop::SurfaceProperties::surface,
      &object3d_prop::SurfaceProperties::surfaceChanged,
      &PositionInterface::adjustedPositionChanged, this,
      &Surface::triggerRendering);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::SurfaceProperties::surface,
      &object3d_prop::SurfaceProperties::surfaceChanged,
      &PositionInterface::adjustedRotationChanged, this,
      &Surface::triggerRendering);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::SurfaceProperties::surface,
      &object3d_prop::SurfaceProperties::surfaceChanged,
      &PositionInterface::adjustedPositionChanged, this,
      &Surface::boundingBoxChanged);
  forwardSignalFromPropertyNode(
      properties, &object3d_prop::SurfaceProperties::surface,
      &object3d_prop::SurfaceProperties::surfaceChanged,
      &PositionInterface::adjustedRotationChanged, this,
      &Surface::boundingBoxChanged);
}
Surface::~Surface() {}

QSharedPointer<Object3DPerShareGroup> Surface::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup) {
  // qDebug() << "Surface::newPerShareGroup()";
  QSharedPointer<SurfacePerShareGroup> ptr(
      new SurfacePerShareGroup(shareGroup),
      [](QObject* obj) { obj->deleteLater(); });

  QPointer<Surface> surfaceWeak = this;

  QObject::connect(
      properties, &object3d_prop::SurfaceProperties::surfaceChanged, ptr.data(),
      [obj = ptr.data(), surfaceWeak](Node* surfaceObj) {
        Q_UNUSED(surfaceObj);
        obj->data.reset();
        if (surfaceWeak) Q_EMIT surfaceWeak->triggerRendering();
      });

  QObject::connect(
      properties, &object3d_prop::SurfaceProperties::shadingTechniqueChanged,
      ptr.data(), [obj = ptr.data(), surfaceWeak](QString shadingTechnique) {
        Q_UNUSED(shadingTechnique);
        // Make sure the shader is recompiled with the correct
        // shading technique
        // TODO: This probably should not re-upload the surface if
        // not needed by the shading technique
        obj->data.reset();
        if (surfaceWeak) Q_EMIT surfaceWeak->triggerRendering();
      });

  QObject::connect(
      properties, &object3d_prop::SurfaceProperties::surfaceChanged, ptr.data(),
      [psg = ptr.data(), surfaceWeak](Node* obj) {
        auto surfaceObj = dynamic_cast<SurfaceNode*>(obj);
        if (!surfaceObj) return;
        // TODO: this is never disconnected
        // qDebug() << "CONN" << surfaceObj;
        connect(surfaceObj, &DataNode::dataChanged, psg, [psg, surfaceWeak]() {
          // qDebug() << "DATA_CHANGED" << surfaceWeak;
          if (!surfaceWeak) return;

          // TODO: Is this correct?
          // The bounding box change notification code probably should be
          // cleaned up
          Q_EMIT surfaceWeak->boundingBoxChanged();

          auto surfaceObj2 = dynamic_cast<vx::SurfaceNode*>(
              surfaceWeak->properties->surface());
          if (!surfaceObj2) {
            // No surface connected => Nothing to do
            if (psg->data) {
              psg->data.reset();
              Q_EMIT surfaceWeak->triggerRendering();
            }
            return;
          }
          // Always call setSurfaceData(), not uploadData(), because
          // uploadData() will not take changed data into account
          psg->setSurfaceData(surfaceWeak,
                              surfaceObj2);  // Calls uploadData()
        });
      });
  Q_EMIT properties->surfaceChanged(properties->surface());  // TODO: hack

  return ptr;
}

namespace {
void drawBoundingBox(OpenGLDrawWidget::PrimitiveBuffer& drawingBuffer,
                     SurfaceNode* surface) {
  if (!surface) {
    qWarning() << "Failed to load surface data\n";
  } else {
  }

  auto surfaceData = surface->surface();
  if (!surfaceData) {
    qWarning() << "drawBoundingBox: No surface data\n";
    return;
  }
  auto srf = qSharedPointerDynamicCast<SurfaceDataTriangleIndexed>(surfaceData);
  if (!srf) {
    qWarning() << "Not a SurfaceDataTriangleIndexed, ignoring";
    return;
  }

  auto size = srf->size();
  auto origin = srf->origin();

  auto x = origin.x();
  auto y = origin.y();
  auto z = origin.z();
  auto xD = size.x();
  auto yD = size.y();
  auto zD = size.z();

  // Get all corners of the cube
  auto a = origin;
  auto b = QVector3D(x + xD, y, z);
  auto c = QVector3D(x, y, z + zD);
  auto d = QVector3D(x, y + yD, z);
  auto e = origin + size;
  auto f = e + QVector3D(0.0f, 0.0f, -zD);
  auto g = b + QVector3D(0.0f, 0.0f, zD);
  auto h = d + QVector3D(0.0f, 0.0f, zD);

  // TODO
  /*
  QVector3D color =
      (surface == selectedSurface() ? selectedBoxColor : boxColor);
  */
  // TODO
  QVector3D DEFAULT_BOX_COLOR = QVector3D(0.5f, 0.5f, 0.5f);
  /*
  QVector3D DEFAULT_SELECTED_BOX_COLOR =
    QVector3D(1.0f, 0.5f, 0.0f);
  QVector4D DEFAULT_DATA_POINT_COLOR =
    QVector4D(0.0f, 1.0f, 1.0f, 1.0f);
  */
  QVector3D boxColor = DEFAULT_BOX_COLOR;
  // QVector3D selectedBoxColor = DEFAULT_SELECTED_BOX_COLOR;
  QVector3D color = boxColor;

  // Draw all edges
  drawingBuffer.addLine(color, a, b);
  drawingBuffer.addLine(color, a, c);
  drawingBuffer.addLine(color, a, d);
  drawingBuffer.addLine(color, f, b);
  drawingBuffer.addLine(color, f, e);
  drawingBuffer.addLine(color, f, d);
  drawingBuffer.addLine(color, g, c);
  drawingBuffer.addLine(color, g, e);
  drawingBuffer.addLine(color, g, b);
  drawingBuffer.addLine(color, h, c);
  drawingBuffer.addLine(color, h, e);
  drawingBuffer.addLine(color, h, d);
}
}  // namespace

void Surface::render(const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
                     const QSharedPointer<Object3DPerContext>& perContext,
                     Object3DRenderContext& context) {
  Q_UNUSED(perContext);
  // qDebug() << "Render Surface";

  auto surfaceObj = dynamic_cast<vx::SurfaceNode*>(this->properties->surface());
  if (!surfaceObj)  // No surface connected => Nothing to do
    return;

  if (!properties->visible()) return;

  auto psg = qSharedPointerDynamicCast<SurfacePerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a SurfacePerShareGroup");

  if (!psg->data) psg->setSurfaceData(this, surfaceObj);

  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  psg->functions.glEnable(GL_DEPTH_TEST);
  psg->functions.glDepthFunc(GL_LEQUAL);
  psg->functions.glFrontFace(GL_CCW);

  auto culling = properties->faceCulling();
  if (culling == "de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.None") {
    psg->functions.glDisable(GL_CULL_FACE);
  } else if (culling ==
             "de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.ShowFront") {
    psg->functions.glEnable(GL_CULL_FACE);
    psg->functions.glCullFace(GL_BACK);
  } else if (culling ==
             "de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.ShowBack") {
    psg->functions.glEnable(GL_CULL_FACE);
    psg->functions.glCullFace(GL_FRONT);
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Invalid value for culling");
  }

  psg->functions.glUseProgram(psg->shaders->renderTriangles().programId());

  vao.bind();

  // Draw all visible triangles:
  auto surface = surfaceObj;
  auto surfaceData = psg->data;
  if (!surfaceData || !surfaceData->isVisible()) return;

  auto indexBuffer = surfaceData->getIndexBuffer();
  auto vertexBuffer = surfaceData->getVertexBuffer();
  auto normalBuffer = surfaceData->getNormalBuffer();

  // 1st attribute buffer : vertices
  psg->functions.glEnableVertexAttribArray(
      psg->shaders->renderTriangles_vertexPosition_modelspace());
  if (!vertexBuffer->bind()) {
    qCritical() << "Binding surface vertex buffer failed";
    return;
  }
  psg->functions.glVertexAttribPointer(
      psg->shaders->renderTriangles_vertexPosition_modelspace(),  // attribute
      3,                                                          // size
      GL_FLOAT,                                                   // type
      GL_FALSE,                                                   // normalized?
      0,                                                          // stride
      (void*)0  // array buffer offset
  );
  vertexBuffer->release();

  // 2nd attribute buffer : normals
  psg->functions.glEnableVertexAttribArray(
      psg->shaders->renderTriangles_vertexNormal());

  if (!normalBuffer->bind()) {
    qCritical() << "Binding surface normal buffer failed";
    return;
  }
  psg->functions.glVertexAttribPointer(
      psg->shaders->renderTriangles_vertexNormal(),  // attribute
      3,                                             // size
      GL_FLOAT,                                      // type
      GL_FALSE,                                      // normalized?
      0,                                             // stride
      (void*)0                                       // array buffer offset
  );
  normalBuffer->release();

  // surface attributes
  auto& attributeBuffer = surfaceData->attributeBuffer();
  for (auto entry : attributeBuffer) {
    auto name = entry.first;
    auto buffer = entry.second;
    QString nameEscaped = name;
    nameEscaped.replace('.', '_');

    auto surfaceAttributes = surfaceData->getSurface()->getAttribute(name);

    auto attributLocation = psg->functions.glGetAttribLocation(
        psg->shaders->renderTriangles().programId(),
        nameEscaped.toStdString().c_str());

    psg->functions.glEnableVertexAttribArray(attributLocation);
    if (!buffer->bind()) {
      qCritical() << "Binding surface attributes buffer failed";
      return;
    }
    // TODO: Should 'normalized' be true? It should be for colors, not sure
    // about other attributes.
    psg->functions.glVertexAttribPointer(
        attributLocation,                     // attribute
        surfaceAttributes->componentCount(),  // size
        surfaceAttributes->getOpenGLType(),   // type
        GL_TRUE,                              // normalized?
        0,                                    // stride
        (void*)0                              // array buffer offset
    );
    buffer->release();
  }

  QMatrix4x4 viewProj = context.projectionMatrix() * context.viewMatrix();

  auto position = surface->getAdjustedPosition();
  auto rotation = surface->getAdjustedRotation();
  QMatrix4x4 model;
  model.translate(position.x(), position.y(), position.z());
  model.rotate(rotation);
  QMatrix4x4 modelViewProj = viewProj * model;

  psg->functions.glUniformMatrix4fv(psg->shaders->renderTriangles_M(), 1,
                                    GL_FALSE, model.constData());
  psg->functions.glUniformMatrix4fv(psg->shaders->renderTriangles_MVP(), 1,
                                    GL_FALSE, modelViewProj.constData());

  // light comming from the direction of the camera
  QVector4D lightPosition = context.lightPosition();
  psg->functions.glUniform4fv(psg->shaders->renderTriangles_lightPosition(), 1,
                              &lightPosition[0]);

  // Set highlighted triangle
  if (properties->highlightCurrentTriangle() && highlightedTriangleID != 0)
    psg->functions.glUniform1i(
        psg->shaders->renderTriangles_highlightedTriangle(),
        highlightedTriangleID);
  else
    psg->functions.glUniform1i(
        psg->shaders->renderTriangles_highlightedTriangle(), 0);

  // Set front and back color (used if surface contains no color information)
  auto frontColor = properties->frontColor();
  psg->functions.glUniform4f(psg->shaders->renderTriangles_defaultFrontColor(),
                             frontColor.red(), frontColor.green(),
                             frontColor.blue(), frontColor.alpha());
  auto backColor = properties->backColor();
  psg->functions.glUniform4f(psg->shaders->renderTriangles_defaultBackColor(),
                             backColor.red(), backColor.green(),
                             backColor.blue(), backColor.alpha());

  // Set color inversion
  psg->functions.glUniform1i(psg->shaders->renderTriangles_invertColor(),
                             context.highlightedNode() == this);

  //            setupCuttingPlanes(dataSet,
  //            cuttingPlanesByVolumeObject[dataSet]);
  psg->setupCuttingPlanes(context.clippingPlanes());

  if (!indexBuffer->bind()) {
    qCritical() << "Binding surface index buffer failed";
    // pickFrameBuffer->unbind();
    return;
  }

  // psg->functions.glDrawArrays(GL_TRIANGLES, 0, surfaceData->numVertices());
  psg->functions.glDrawElements(
      GL_TRIANGLES, surfaceData->getSurface()->triangles().size() * 3,
      GL_UNSIGNED_INT, (void*)0);
  psg->functions.glDisableVertexAttribArray(
      psg->shaders->renderTriangles_vertexPosition_modelspace());
  psg->functions.glDisableVertexAttribArray(
      psg->shaders->renderTriangles_vertexNormal());

  for (auto entry : attributeBuffer) {
    auto name = entry.first;
    QString nameEscaped = name;
    nameEscaped.replace('.', '_');
    auto attributLocation = psg->functions.glGetAttribLocation(
        psg->shaders->renderTriangles().programId(),
        nameEscaped.toStdString().c_str());
    psg->functions.glDisableVertexAttribArray(attributLocation);
  }

  vao.release();

  psg->functions.glUseProgram(0);

  psg->functions.glDisable(GL_CULL_FACE);

  // Draw axis indicators
  OpenGLDrawWidget::PrimitiveBuffer drawingBuffer;

  auto srf =
      qSharedPointerDynamicCast<SurfaceDataTriangleIndexed>(surface->data());
  if (!srf) {
    qWarning() << "Surface data is not a SurfaceDataTriangleIndexed";
    return;
  }

  auto size = srf->size() / 50.0f;
  auto origin = srf->origin() / 50.0f;  // TODO: this does not really make sense

  // Visualize the origin of the coordinate system by drawing the axis lines
  if (properties->drawOrigin()) {
    float halfLength = srf->diagonalSize() / 50.0f;

    QVector3D min = context.boundingBox().min();
    QVector3D max = context.boundingBox().max();

    float start, end;

    start = fmin(-halfLength, min.x());
    end = fmax(halfLength, max.x());

    if (true /*TODO !canFilterAxis || !axisFilter->filterX()*/) {
      drawingBuffer.addLine(QVector3D(1.0f, 0.0f, 0.0f),
                            QVector3D(start, 0.0f, 0.0f),
                            QVector3D(end, 0.0f, 0.0f));
    }

    if (true /*TODO !canFilterAxis || !axisFilter->filterY()*/) {
      start = fmin(-halfLength, min.y());
      end = fmax(halfLength, max.y());

      drawingBuffer.addLine(QVector3D(0.0f, 1.0f, 0.0f),
                            QVector3D(0.0f, start, 0.0f),
                            QVector3D(0.0f, end, 0.0f));
    }

    if (true /*TODO !canFilterAxis || !axisFilter->filterZ()*/) {
      start = fmin(-halfLength, min.z());
      end = fmax(halfLength, max.z());

      drawingBuffer.addLine(QVector3D(0.0f, 0.0f, 1.0f),
                            QVector3D(0.0f, 0.0f, start),
                            QVector3D(0.0f, 0.0f, end));
    }
  }

  // Draw axis arrows at the center of the main dataset
  if (properties->drawAxisArrows()) {
    // TODO: Draw for every dataset
    auto dataCenter = origin + size * 0.5f;

    drawingBuffer.addLine(QVector3D(0.0f, 1.0f, 0.0f), dataCenter,
                          dataCenter + QVector3D(size.x(), 0.0f, 0.0f));

    drawingBuffer.addLine(QVector3D(0.0f, 0.0f, 1.0f), dataCenter,
                          dataCenter + QVector3D(0.0f, size.y(), 0.0f));

    drawingBuffer.addLine(QVector3D(1.0f, 0.0f, 0.0f), dataCenter,
                          dataCenter + QVector3D(0.0f, 0.0f, size.z()));
  }

  psg->drawUtils->draw(drawingBuffer, viewProj);

  if (properties->drawBoundingBox()) {
    QMatrix4x4 matrix = viewProj;
    matrix.translate(surface->getAdjustedPosition());
    matrix.rotate(surface->getAdjustedRotation());
    OpenGLDrawWidget::PrimitiveBuffer drawingBuffer2;
    drawBoundingBox(drawingBuffer2, surface);
    psg->drawUtils->draw(drawingBuffer2, matrix);
  }
}

// TODO: Reduce code duplication with render()
void Surface::renderPick(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context, Object3DPickImageInfo& pickInfo) {
  Q_UNUSED(perContext);
  // qDebug() << "Render Surface (pick image)";

  auto surfaceObj = dynamic_cast<vx::SurfaceNode*>(this->properties->surface());
  if (!surfaceObj)  // No surface connected => Nothing to do
    return;

  if (!properties->visible()) return;

  auto psg = qSharedPointerDynamicCast<SurfacePerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a SurfacePerShareGroup");

  if (!psg->data) psg->setSurfaceData(this, surfaceObj);

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  // Create new vao to make sure it is valid for the current context
  QOpenGLVertexArrayObject vao;
  if (!vao.create()) {
    // return "Creating VAO failed";
    qWarning() << "Creating VAO failed";
  }

  psg->functions.glEnable(GL_DEPTH_TEST);
  psg->functions.glDepthFunc(GL_LEQUAL);

  auto culling = properties->faceCulling();
  if (culling == "de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.None") {
    psg->functions.glDisable(GL_CULL_FACE);
  } else if (culling ==
             "de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.ShowFront") {
    psg->functions.glEnable(GL_CULL_FACE);
    psg->functions.glFrontFace(GL_CCW);
  } else if (culling ==
             "de.uni_stuttgart.Voxie.Object3D.Surface.FaceCulling.ShowBack") {
    psg->functions.glEnable(GL_CULL_FACE);
    psg->functions.glFrontFace(GL_CW);
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Invalid value for culling");
  }

  psg->functions.glUseProgram(psg->shaders->pickTriangles().programId());

  vao.bind();

  // Draw all visible triangles:
  auto surface = surfaceObj;
  auto surfaceData = psg->data;
  if (!surfaceData || !surfaceData->isVisible()) return;

  auto indexBuffer = surfaceData->getIndexBuffer();
  auto vertexBuffer = surfaceData->getVertexBuffer();
  // auto colorBuffer = surfaceData->getColorBuffer();

  // 1st attribute buffer : vertices
  psg->functions.glEnableVertexAttribArray(
      psg->shaders->pickTriangles_vertexPosition_modelspace());
  if (!vertexBuffer->bind()) {
    qCritical() << "Binding surface vertex buffer failed";
    return;
  }
  psg->functions.glVertexAttribPointer(
      psg->shaders->pickTriangles_vertexPosition_modelspace(),  // attribute
      3,                                                        // size
      GL_FLOAT,                                                 // type
      GL_FALSE,                                                 // normalized?
      0,                                                        // stride
      (void*)0  // array buffer offset
  );
  vertexBuffer->release();

  /*
  // 2nd attribute buffer : colors
  psg->functions.glEnableVertexAttribArray(
      psg->shaders->pickTriangles_vertexColor());
  if (!colorBuffer->bind()) {
    qCritical() << "Binding surface color buffer failed";
    return;
  }
  psg->functions.glVertexAttribPointer(
      psg->shaders->pickTriangles_vertexColor(),  // attribute
      3,                                            // size
      GL_FLOAT,                                     // type
      GL_FALSE,                                     // normalized?
      0,                                            // stride
      (void*)0                                      // array buffer offset
  );
  colorBuffer->release();
  */

  auto position = surface->getAdjustedPosition();
  auto rotation = surface->getAdjustedRotation();
  auto matrix = matViewProj;

  matrix.translate(position.x(), position.y(), position.z());
  matrix.rotate(rotation);
  psg->functions.glUniformMatrix4fv(psg->shaders->pickTriangles_MVP(), 1,
                                    GL_FALSE, matrix.constData());

  auto pickData = createQSharedPointer<SurfacePickData>(this);
  pickData->data = psg->data;  // TODO

  auto datasetId = pickInfo.addData(pickData);

  psg->functions.glUniform1i(psg->shaders->pickTriangles_gObjectIndex(),
                             datasetId);
  psg->functions.glUniform1i(psg->shaders->pickTriangles_gDrawIndex(), 1);

  // TODO: this is also needed for the pick image
  // psg->setupCuttingPlanes(context.clippingPlanes()); // TODO

  if (!indexBuffer->bind()) {
    qCritical() << "Binding surface index buffer failed";
    // pickFrameBuffer->unbind();
    return;
  }

  // psg->functions.glDrawArrays(GL_TRIANGLES, 0, surfaceData->numVertices());
  psg->functions.glDrawElements(
      GL_TRIANGLES, surfaceData->getSurface()->triangles().size() * 3,
      GL_UNSIGNED_INT, (void*)0);
  psg->functions.glDisableVertexAttribArray(
      psg->shaders->pickTriangles_vertexPosition_modelspace());
  /*
  psg->functions.glDisableVertexAttribArray(
      psg->shaders->pickTriangles_vertexColor());
  */

  vao.release();

  psg->functions.glUseProgram(0);

  psg->functions.glDisable(GL_CULL_FACE);
}

BoundingBox3D Surface::getBoundingBox() {
  auto surface = dynamic_cast<SurfaceNode*>(properties->surface());
  if (!surface) {
    return BoundingBox3D::empty();
  } else {
    auto surfaceData = surface->surface();
    if (!surfaceData) return BoundingBox3D::empty();
    auto srf =
        qSharedPointerDynamicCast<SurfaceDataTriangleIndexed>(surfaceData);
    if (!srf) {
      qWarning() << "Not a SurfaceDataTriangleIndexed, ignoring";
      return BoundingBox3D::empty();
    }

    auto origin = srf->origin();
    auto size = srf->size();
    // return BoundingBox3D::point(origin) + BoundingBox3D::point(origin +
    // size);
    auto bb = BoundingBox3D::empty();
    // Add all corners to the bounding box
    for (int x = 0; x < 2; x++) {
      for (int y = 0; y < 2; y++) {
        for (int z = 0; z < 2; z++) {
          // Position in object coordinate system
          auto posObject = origin + QVector3D(x, y, z) * size;
          // Position in global coordinate system
          auto posGlobal =
              surface->getAdjustedPosition() +
              surface->getAdjustedRotation().rotatedVector(posObject);
          bb += BoundingBox3D::point(posGlobal);
        }
      }
    }
    return bb;
  }
}

bool Surface::needMouseTracking() {
  return properties->highlightCurrentTriangle();
}

void Surface::mouseMoveEventImpl(
    QMouseEvent* event,
    std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
    const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd) {
  Q_UNUSED(event);
  Q_UNUSED(mouseRayStart);
  Q_UNUSED(mouseRayEnd);

  auto selectedNodes =
      vx::voxieRoot().activeVisualizerProvider()->selectedNodes();

  if (properties->highlightCurrentTriangle()) {
    auto oldHighlightedTriangleID = highlightedTriangleID;

    if (std::get<0>(pickData) && std::get<0>(pickData)->object() == this) {
      highlightedTriangleID = std::get<2>(pickData);
    } else {
      highlightedTriangleID = 0;
      // qDebug() << "No hit";
    }
    if (oldHighlightedTriangleID != highlightedTriangleID)
      Q_EMIT triggerRendering();  // TODO: don't update pick image
  }
}
void Surface::mousePressEventImpl(
    QMouseEvent* event,
    std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
    const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd) {
  // qDebug() << "mousePressEventImpl" << std::get<0>(pickData) << event;

  // Ignore all non-left button presses
  if (event->button() != Qt::LeftButton) return;

  // Ignore all button presses if another button is already pressed
  if ((event->buttons() & (Qt::MiddleButton | Qt::RightButton)) != Qt::NoButton)
    return;

  auto modifiers =
      event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

  if (modifiers == Qt::ShiftModifier) {
    mouseSelectEvent(event, pickData, mouseRayStart, mouseRayEnd, false);
  } else if (modifiers == Qt::ControlModifier) {
    mouseSelectEvent(event, pickData, mouseRayStart, mouseRayEnd, true);
  }
}
// TODO: Clean up, use different methods for selecting and setting points?
void Surface::mouseSelectEvent(
    QMouseEvent* event,
    std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
    const QVector3D& mouseRayStartGlobal, const QVector3D& mouseRayEndGlobal,
    bool doSetPoint) {
  Q_UNUSED(event);

  QVector3D hitPos;

  // No object or another one has been hit
  if (!std::get<0>(pickData) || std::get<0>(pickData)->object() != this) return;

  // qDebug() << "Hit" << std::get<0>(pickData) << std::get<1>(pickData) <<
  // std::get<2>(pickData);

  auto data = dynamic_cast<SurfacePickData*>(std::get<0>(pickData));
  if (!data) {
    qWarning() << "Not a SurfacePickData";
    return;
  }
  auto info = data->data;
  uint32_t triId = std::get<2>(pickData);
  auto surf = info->getSurface();
  if (triId == 0) {
    qWarning() << "triId == 0";
    return;
  } else if (triId > info->getSurface()->triangles().size()) {
    qWarning() << "triId > info->getSurface()->triangles().size()";
    return;
  }

  // Get Triangle
  auto tri = surf->triangles()[triId - 1];
  // qDebug() << std::get<1>(pickData) << tri[0] << tri[1] << tri[2];
  auto vert1 = surf->vertices()[tri[0]];
  auto vert2 = surf->vertices()[tri[1]];
  auto vert3 = surf->vertices()[tri[2]];
  auto d1 = vert2 - vert1;
  auto d2 = vert3 - vert1;
  // Get triangle normal
  auto normal = QVector3D::crossProduct(d1.normalized(), d2.normalized());
  normal.normalize();
  // Get rotation which will rotation the triangle into a coordinate
  // system where all the triangle vertices have the same z coordinate
  auto rot = QQuaternion::rotationTo(normal, QVector3D(0, 0, 1));
  auto rotVert1 = rot.rotatedVector(vert1);
  auto rotVert2 = rot.rotatedVector(vert2);
  auto rotVert3 = rot.rotatedVector(vert3);
  double z = (rotVert1.z() + rotVert2.z() + rotVert3.z()) / 3.0;
  auto surface = info->getSurfaceNode();
  // Transform mouse ray from global into object coordinate system
  /*
  QVector3D mouseRayStart =
      surface->getAdjustedPosition() +
      surface->getAdjustedRotation().rotatedVector(mouseRayStartGlobal);
  QVector3D mouseRayEnd =
      surface->getAdjustedPosition() +
      surface->getAdjustedRotation().rotatedVector(mouseRayEndGlobal);
  */
  QVector3D mouseRayStart =
      surface->getAdjustedRotation().inverted().rotatedVector(
          mouseRayStartGlobal - surface->getAdjustedPosition());
  QVector3D mouseRayEnd =
      surface->getAdjustedRotation().inverted().rotatedVector(
          mouseRayEndGlobal - surface->getAdjustedPosition());
  auto mouseRayStartRot = rot.rotatedVector(mouseRayStart);
  auto mouseRayEndRot = rot.rotatedVector(mouseRayEnd);
  auto mouseRayDir = (mouseRayEndRot - mouseRayStartRot).normalized();
  // Intersect the mouse ray with the plane the triangle is on
  auto factor = (z - mouseRayStartRot.z()) / mouseRayDir.z();
  auto pos = mouseRayStartRot + factor * mouseRayDir;
  // Transform the resulting coordinate back
  auto objPos = rot.inverted().rotatedVector(pos);
  hitPos = objPos;

  auto position = surface->getAdjustedPosition();
  auto rotation = surface->getAdjustedRotation();
  hitPos = rotation.rotatedVector(hitPos);
  hitPos += position;

  if (doSetPoint) {
    Q_EMIT this->setPointCalled(hitPos);
  } else {
    auto selectedNodes =
        vx::voxieRoot().activeVisualizerProvider()->selectedNodes();
    if (!selectedNodes.contains(this)) {
      QList<Node*> nodes;
      nodes << this;
      vx::voxieRoot().activeVisualizerProvider()->setSelectedNodes(nodes);
    }
  }
}

vx::SurfaceNode* Surface::surface() {
  return dynamic_cast<vx::SurfaceNode*>(properties->surface());
}
QString Surface::shadingTechnique() { return properties->shadingTechnique(); }

NODE_PROTOTYPE_IMPL_SEP(object3d_prop::Surface, vis3d::Surface)
