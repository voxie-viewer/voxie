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

#include "Grid.hpp"

#include <PluginVis3D/Grid3D.hpp>
#include <PluginVis3D/Grid3DWidget.hpp>
#include <PluginVis3D/Prototypes.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Vis/OpenGLWidget.hpp>

using namespace vx;
using namespace vx::visualization;
using namespace vx::vis3d;

namespace {
class GridPerShareGroup : public Object3DPerShareGroup {
 public:
  OpenGLDrawUtils utils;

  QOpenGLFunctions functions;
};
}  // namespace

Grid::Grid()
    : Object3DNode(getPrototypeSingleton()),
      properties(new GridProperties(this)) {
  grid3D = new Grid3D(this);

  // TODO: expose this as properties
  Grid3DWidget* gridWid = new Grid3DWidget(nullptr, grid3D);
  gridWid->setObjectName("GridWidget");
  gridWid->setWindowTitle("Grid");

  this->addPropertySection(gridWid);

  connect(grid3D, &Grid3D::gridChanged, this, &Object3DNode::triggerRendering);
}
Grid::~Grid() {}

QSharedPointer<Object3DPerShareGroup> Grid::newPerShareGroup(
    const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup) {
  Q_UNUSED(shareGroup);
  // qDebug() << "Grid::newPerShareGroup()";
  QSharedPointer<GridPerShareGroup> ptr(
      new GridPerShareGroup, [](QObject* obj) { obj->deleteLater(); });
  ptr->functions.initializeOpenGLFunctions();
  ptr->utils.initialize();
  return ptr;
}

void Grid::render(const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
                  const QSharedPointer<Object3DPerContext>& perContext,
                  Object3DRenderContext& context) {
  Q_UNUSED(perContext);
  // qDebug() << "Render Grid";

  auto psg = qSharedPointerDynamicCast<GridPerShareGroup>(perShareGroup);
  if (!psg)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a GridPerShareGroup");

  QMatrix4x4 matViewProj = context.projectionMatrix() * context.viewMatrix();

  OpenGLDrawUtils::PrimitiveBuffer drawingBuffer;

  auto preferedLength = context.pixelSize() * 30;
  auto bb = context.boundingBox();
  OpenGLDrawUtils::PrimitiveBuffer buffer;
  this->grid3D->drawGrid(buffer, bb.min(), bb.max(), preferedLength);

  psg->functions.glEnable(GL_BLEND);
  psg->functions.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  psg->functions.glDepthMask(GL_FALSE);
  psg->utils.draw(buffer, matViewProj);
  psg->functions.glDepthMask(GL_TRUE);
  psg->functions.glDisable(GL_BLEND);
}

BoundingBox3D Grid::getBoundingBox() {
  // The grid 3D object does not influence the bounding box
  return BoundingBox3D::empty();
}

NODE_PROTOTYPE_IMPL(Grid)
