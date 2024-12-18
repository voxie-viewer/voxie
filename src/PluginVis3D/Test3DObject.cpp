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

#include "Test3DObject.hpp"

#include <PluginVis3D/Prototypes.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Vis/OpenGLWidget.hpp>

VX_NODE_INSTANTIATION(vx::Test3DObject)

using namespace vx;
using namespace vx::visualization;

namespace {
class Test3DObjectPerShareGroup : public Object3DPerShareGroup {
 public:
  OpenGLDrawUtils utils;
};
}  // namespace

Test3DObject::Test3DObject()
    : Object3DNode(getPrototypeSingleton()),
      properties(new Test3DObjectProperties(this)) {
  QObject::connect(properties, &Test3DObjectProperties::lengthChanged, this,
                   &Test3DObject::boundingBoxChanged);
  QObject::connect(properties, &Test3DObjectProperties::lengthChanged, this,
                   &Test3DObject::triggerRendering);
}
Test3DObject::~Test3DObject() {}

QSharedPointer<Object3DPerShareGroup> Test3DObject::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup) {
  Q_UNUSED(shareGroup);
  // qDebug() << "Test3DObject::newPerShareGroup()";
  QSharedPointer<Test3DObjectPerShareGroup> ptr(
      new Test3DObjectPerShareGroup, [](QObject* obj) { obj->deleteLater(); });
  ptr->utils.initialize();
  return ptr;
}

void Test3DObject::render(
    const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
    const QSharedPointer<Object3DPerContext>& perContext,
    Object3DRenderContext& context) {
  Q_UNUSED(perContext);
  // qDebug() << "Render Test3DObject";

  auto psg =
      qSharedPointerDynamicCast<Test3DObjectPerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a Test3DObjectPerShareGroup");

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  OpenGLDrawUtils::PrimitiveBuffer drawingBuffer;

  auto val = properties->length();

  drawingBuffer.addLine(vx::Color(1.0f, 0.0f, 0.0f),
                        QVector3D(-val, -val, -val), QVector3D(val, val, val));

  psg->utils.draw(drawingBuffer, matViewProj);
}

BoundingBox3D Test3DObject::getBoundingBox() {
  auto val = properties->length();
  return BoundingBox3D::point(vx::Vector<double, 3>(-val, -val, -val)) +
         BoundingBox3D::point(vx::Vector<double, 3>(val, val, val));
}
