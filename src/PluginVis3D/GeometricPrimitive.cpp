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

#include "GeometricPrimitive.hpp"

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <Voxie/MathQt.hpp>

#include <Voxie/Node/PropertyHelper.hpp>

#include <PluginVis3D/Prototypes.hpp>

#include <QtGui/QOpenGLFunctions>

VX_NODE_INSTANTIATION(vx::vis3d::GeometricPrimitive)

using namespace vx;
using namespace vx::visualization;
using namespace vx::vis3d;

namespace {
class GeometricPrimitivePerShareGroup : public Object3DPerShareGroup {
 public:
  QSharedPointer<Object3DRenderContextPerShareGroup> shareGroup;

  QOpenGLFunctions functions;

  QSharedPointer<OpenGLDrawUtils> drawUtils;

  GeometricPrimitivePerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup)
      : shareGroup(shareGroup) {
    functions.initializeOpenGLFunctions();

    drawUtils.reset(new OpenGLDrawUtils);
    drawUtils->initialize();
  }
};
}  // namespace

namespace vx {
namespace vis3d {
GeometricPrimitive::GeometricPrimitive()
    : Object3DNode(getPrototypeSingleton()),
      properties(new object3d_prop::GeometricPrimitiveProperties(this)) {
  color = QVector4D(0.0f, 1.0f, 1.0f, 1.0f);
  colorHighlight = QVector4D(0.2f, 0.75f, 0.2f, 1.0f);
  colorMeasuringLine = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);

  forwardSignalFromPropertyNodeOnReconnect(
      properties,
      &object3d_prop::GeometricPrimitiveProperties::geometricPrimitive,
      &object3d_prop::GeometricPrimitiveProperties::geometricPrimitiveChanged,
      &GeometricPrimitiveNode::measurementPrimitivesChanged, this,
      &GeometricPrimitive::newMeasurement);
  forwardSignalFromPropertyNodeOnReconnect(
      properties,
      &object3d_prop::GeometricPrimitiveProperties::geometricPrimitive,
      &object3d_prop::GeometricPrimitiveProperties::geometricPrimitiveChanged,
      &GeometricPrimitiveNode::selectedPrimitiveChanged, this,
      &GeometricPrimitive::currentPointChanged);
  forwardSignalFromPropertyNodeOnReconnect(
      properties,
      &object3d_prop::GeometricPrimitiveProperties::geometricPrimitive,
      &object3d_prop::GeometricPrimitiveProperties::geometricPrimitiveChanged,
      &DataNode::dataChangedFinished, this,
      &GeometricPrimitive::gpoDataChangedFinished);

  QObject::connect(this, &GeometricPrimitive::gpoDataChangedFinished, this,
                   &Object3DNode::boundingBoxChanged);

  QObject::connect(this, &GeometricPrimitive::newMeasurement, this,
                   &Object3DNode::triggerRendering);
  QObject::connect(this, &GeometricPrimitive::currentPointChanged, this,
                   &Object3DNode::triggerRendering);
  QObject::connect(this, &GeometricPrimitive::gpoDataChangedFinished, this,
                   &Object3DNode::triggerRendering);
}
GeometricPrimitive::~GeometricPrimitive() {}

QSharedPointer<Object3DPerShareGroup> GeometricPrimitive::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup) {
  QSharedPointer<GeometricPrimitivePerShareGroup> ptr(
      new GeometricPrimitivePerShareGroup(shareGroup),
      [](QObject* obj) { obj->deleteLater(); });
  return ptr;
}

void GeometricPrimitive::constructDataPointMarker(
    OpenGLDrawUtils::PrimitiveBuffer& drawingBuffer,
    const QSharedPointer<GeometricPrimitivePoint>& point, float length,
    bool isCurrentPoint) {
  QVector3D center = point->position();
  QVector4D dataPointColor = color;
  if (isCurrentPoint) dataPointColor = colorHighlight;
  float hl = length * 0.1f;

  // Constructing X pyramids
  {
    QVector3D a = center + QVector3D(length, hl, hl);
    QVector3D b = center + QVector3D(length, -hl, hl);
    QVector3D c = center + QVector3D(length, -hl, -hl);
    QVector3D d = center + QVector3D(length, hl, -hl);

    drawingBuffer.addTriangle(dataPointColor, center, a, b);
    drawingBuffer.addTriangle(dataPointColor, center, b, c);
    drawingBuffer.addTriangle(dataPointColor, center, c, d);
    drawingBuffer.addTriangle(dataPointColor, center, d, a);
    drawingBuffer.addQuad(dataPointColor, a, b, c, d);

    // Construct opposite side:
    auto opLength = center.x() - length;
    a.setX(opLength);
    b.setX(opLength);
    c.setX(opLength);
    d.setX(opLength);

    drawingBuffer.addTriangle(dataPointColor, center, a, b);
    drawingBuffer.addTriangle(dataPointColor, center, b, c);
    drawingBuffer.addTriangle(dataPointColor, center, c, d);
    drawingBuffer.addTriangle(dataPointColor, center, d, a);
    drawingBuffer.addQuad(dataPointColor, a, b, c, d);
  }

  // Constructing Y Pyramids
  {
    QVector3D a = center + QVector3D(hl, length, hl);
    QVector3D b = center + QVector3D(-hl, length, hl);
    QVector3D c = center + QVector3D(-hl, length, -hl);
    QVector3D d = center + QVector3D(hl, length, -hl);
    drawingBuffer.addTriangle(dataPointColor, center, a, b);
    drawingBuffer.addTriangle(dataPointColor, center, b, c);
    drawingBuffer.addTriangle(dataPointColor, center, c, d);
    drawingBuffer.addTriangle(dataPointColor, center, d, a);
    drawingBuffer.addQuad(dataPointColor, a, b, c, d);
    // Construct opposite side:
    auto opLength = center.y() - length;
    a.setY(opLength);
    b.setY(opLength);
    c.setY(opLength);
    d.setY(opLength);

    drawingBuffer.addTriangle(dataPointColor, center, a, b);
    drawingBuffer.addTriangle(dataPointColor, center, b, c);
    drawingBuffer.addTriangle(dataPointColor, center, c, d);
    drawingBuffer.addTriangle(dataPointColor, center, d, a);
    drawingBuffer.addQuad(dataPointColor, a, b, c, d);
  }
  // Constructing Z pyramids
  {
    QVector3D a = center + QVector3D(hl, hl, length);
    QVector3D b = center + QVector3D(-hl, hl, length);
    QVector3D c = center + QVector3D(-hl, -hl, length);
    QVector3D d = center + QVector3D(hl, -hl, length);
    drawingBuffer.addTriangle(dataPointColor, center, a, b);
    drawingBuffer.addTriangle(dataPointColor, center, b, c);
    drawingBuffer.addTriangle(dataPointColor, center, c, d);
    drawingBuffer.addTriangle(dataPointColor, center, d, a);
    drawingBuffer.addQuad(dataPointColor, a, b, c, d);
    // Construct opposite side:
    auto opLength = center.z() - length;
    a.setZ(opLength);
    b.setZ(opLength);
    c.setZ(opLength);
    d.setZ(opLength);

    drawingBuffer.addTriangle(dataPointColor, center, a, b);
    drawingBuffer.addTriangle(dataPointColor, center, b, c);
    drawingBuffer.addTriangle(dataPointColor, center, c, d);
    drawingBuffer.addTriangle(dataPointColor, center, d, a);
    drawingBuffer.addQuad(dataPointColor, a, b, c, d);
  }
}

void GeometricPrimitive::render(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context) {
  Q_UNUSED(perContext);
  // qDebug() << "Render GeometricPrimitive";

  auto psg =
      qSharedPointerDynamicCast<GeometricPrimitivePerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a GeometricPrimitivePerShareGroup");

  if (!properties->visible()) return;

  auto gpo = dynamic_cast<GeometricPrimitiveNode*>(
      this->properties->geometricPrimitive());
  if (!gpo) return;  // No geometricPrimitive connected => Nothing to do
  auto gpd = gpo->geometricPrimitiveData();

  // Set up OpenGL

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  psg->functions.glEnable(GL_DEPTH_TEST);
  psg->functions.glDepthFunc(GL_LEQUAL);
  psg->functions.glDisable(GL_CULL_FACE);

  // Draw data points
  float pointSize = context.pixelSize() * 50.0f;

  if (gpd) {
    OpenGLDrawUtils::PrimitiveBuffer drawingBuffer;
    auto currentPointID = gpo->getNodePropertyTyped<quint64>(
        "de.uni_stuttgart.Voxie.Data.GeometricPrimitive.SelectedPrimitive");
    auto primitives = gpd->primitives();
    for (const auto& id : primitives.keys()) {
      auto primitive = primitives[id];
      auto pointPrimitive =
          qSharedPointerDynamicCast<GeometricPrimitivePoint>(primitive);
      if (!pointPrimitive)  // TODO
        continue;
      this->constructDataPointMarker(drawingBuffer, pointPrimitive, pointSize,
                                     id == currentPointID);
    }
    psg->drawUtils->draw(drawingBuffer, matViewProj);
  }

  // Draw Measuring line
  auto line = gpo->currentMeasurementPoints();
  if (std::get<0>(line)) {
    OpenGLDrawUtils::PrimitiveBuffer buffer;
    buffer.addLine(colorMeasuringLine, std::get<1>(line), std::get<2>(line));
    psg->functions.glEnable(GL_BLEND);
    psg->functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    psg->functions.glDepthMask(GL_FALSE);
    psg->drawUtils->draw(buffer, matViewProj);
    psg->functions.glDepthMask(GL_TRUE);
    psg->functions.glDisable(GL_BLEND);
  }
}

BoundingBox3D GeometricPrimitive::getBoundingBox() {
  BoundingBox3D bb = BoundingBox3D::empty();

  auto gpo = dynamic_cast<GeometricPrimitiveNode*>(
      this->properties->geometricPrimitive());
  if (!gpo) return bb;

  auto line = gpo->currentMeasurementPoints();
  if (std::get<0>(line)) {
    bb += BoundingBox3D::point(vectorCast<double>(toVector(std::get<1>(line))));
    bb += BoundingBox3D::point(vectorCast<double>(toVector(std::get<2>(line))));
  }

  auto gpd = gpo->geometricPrimitiveData();
  if (!gpd) return bb;

  for (const auto& primitive : gpd->primitives()) {
    auto pointPrimitive =
        qSharedPointerDynamicCast<GeometricPrimitivePoint>(primitive);
    if (!pointPrimitive)  // TODO
      continue;
    bb += BoundingBox3D::point(
        vectorCast<double>(toVector(pointPrimitive->position())));
  }
  return bb;
}

GeometricPrimitiveNode* GeometricPrimitive::getGpo() {
  return dynamic_cast<GeometricPrimitiveNode*>(
      properties->geometricPrimitive());
}
void GeometricPrimitive::setGpo(GeometricPrimitiveNode* gpo) {
  properties->setGeometricPrimitive(gpo);
}

}  // namespace vis3d
}  // namespace vx
