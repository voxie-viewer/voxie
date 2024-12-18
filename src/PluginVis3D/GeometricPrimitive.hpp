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

#include <Voxie/Vis/OpenGLWidget.hpp>

#include <PluginVis3D/Prototypes.forward.hpp>

namespace vx {
class GeometricPrimitiveNode;
class GeometricPrimitivePoint;

namespace vis3d {
class GeometricPrimitive : public vx::Object3DNode {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Object3D.GeometricPrimitive")

  // TODO: Should be properties
  QVector4D color;
  QVector4D colorHighlight;
  QVector4D colorMeasuringLine;

  /**
   * @brief drawDataPoint Draws a data point at the given coordinate.
   * @param buffer The buffer to write the constructed polygons to.
   * @param center The point's center coordinate.
   * @param length The length of the point's lines.
   */
  void constructDataPointMarker(
      vx::visualization::OpenGLDrawUtils::PrimitiveBuffer& drawingBuffer,
      const QSharedPointer<GeometricPrimitivePoint>& point, float length,
      bool isCurrentPoint);

 public:
  explicit GeometricPrimitive();
  ~GeometricPrimitive();

  QSharedPointer<Object3DPerShareGroup> newPerShareGroup(
      const QSharedPointer<Object3DRenderContextPerShareGroup>& shareGroup)
      override;

  void render(const QSharedPointer<Object3DPerShareGroup>& perShareGroup,
              const QSharedPointer<Object3DPerContext>& perContext,
              Object3DRenderContext& context) override;

  BoundingBox3D getBoundingBox() override;

  GeometricPrimitiveNode* getGpo();
  void setGpo(GeometricPrimitiveNode* gpo);

 Q_SIGNALS:
  // Signals forwarded from GeometricPrimitiveNode
  void newMeasurement();
  void currentPointChanged();
  void gpoDataChangedFinished();
};
}  // namespace vis3d
}  // namespace vx
