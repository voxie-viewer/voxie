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

#include <Voxie/Node/Object3DNode.hpp>

#include <PluginVis3D/Prototypes.forward.hpp>

namespace vx {
class SurfaceNode;

namespace vis3d {
class Surface : public vx::Object3DNode {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Object3D.Surface")

  void mouseMoveEventImpl(
      QMouseEvent* event,
      std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
      const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd);
  void mousePressEventImpl(
      QMouseEvent* event,
      std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
      const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd);
  void mouseSelectEvent(
      QMouseEvent* event,
      std::tuple<Object3DPickImageData*, uint32_t, uint32_t> pickData,
      const QVector3D& mouseRayStart, const QVector3D& mouseRayEnd,
      bool doSetPoint);

  uint32_t highlightedTriangleID = 0;

 public:
  explicit Surface();
  ~Surface();

  vx::SurfaceNode* surface();
  QString shadingTechnique();

  QSharedPointer<Object3DPerShareGroup> newPerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup)
      override;

  void render(const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
              const QSharedPointer<Object3DPerContext>& perContext,
              Object3DRenderContext& context) override;

  void renderPick(const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
                  const QSharedPointer<Object3DPerContext>& perContext,
                  Object3DRenderContext& context,
                  Object3DPickImageInfo& pickInfo) override;

  BoundingBox3D getBoundingBox() override;

  bool needMouseTracking() override;
};
}  // namespace vis3d
}  // namespace vx
