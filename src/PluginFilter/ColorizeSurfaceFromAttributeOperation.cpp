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

#include "ColorizeSurfaceFromAttributeOperation.hpp"

#include <QtGui/QOpenGLFunctions>

#include <VoxieBackend/Data/SurfaceAttribute.hpp>

using namespace vx;
using namespace vx::io;

ColorizeSurfaceFromAttributeOperation::ColorizeSurfaceFromAttributeOperation(
    const QSharedPointer<RunFilterOperation>& operation)
    : operation(operation) {}

ColorizeSurfaceFromAttributeOperation::
    ~ColorizeSurfaceFromAttributeOperation() {}

void ColorizeSurfaceFromAttributeOperation::colorizeModel(
    vx::SurfaceNode* surfaceNode, QString attributeName,
    vx::Colorizer* colorizer) {
  // TODO: Should maybe check if the selected attribute has as many values as
  // there are vertices in the surface. Otherwise
  //       it doesn't really make sense.

  try {
    auto* surfaceData = dynamic_cast<SurfaceDataTriangleIndexed*>(
        surfaceNode->surface().data());

    if (surfaceData) {
      // create a copy of the vertices and triangles
      std::vector<SurfaceDataTriangleIndexed::Triangle> triangles(
          surfaceData->triangles().size());
      for (size_t i = 0; i < triangles.size(); i++) {
        triangles[i] = surfaceData->triangles()[i];
      }

      std::vector<QVector3D> vertices(surfaceData->vertices().size());
      for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = surfaceData->vertices()[i];
      }

      // the attribute whose values we will use for colorization
      QSharedPointer<SurfaceAttribute> attribute =
          surfaceData->getAttribute(attributeName);

      QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          attributes{
              std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color",
                              attribute->kind(), 4,
                              std::make_tuple("float", 32, "native"), "Color",
                              QMap<QString, QDBusVariant>(),
                              QMap<QString, QDBusVariant>()),
              std::make_tuple(
                  "de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside",
                  attribute->kind(), 4, std::make_tuple("float", 32, "native"),
                  "Backside Color", QMap<QString, QDBusVariant>(),
                  QMap<QString, QDBusVariant>()),

          };

      // the surface we will output later
      auto newSurface = SurfaceDataTriangleIndexed::create(vertices, triangles,
                                                           false, attributes);

      auto attributeColor = newSurface->getAttribute(
          "de.uni_stuttgart.Voxie.SurfaceAttribute.Color");
      auto attributeColorBackside = newSurface->getAttribute(
          "de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside");

      for (quint64 i = 0; i < attribute->getSize(); i++) {
        // if the attribute has integer values we need to get those, otherwise
        // we have to get the float values
        double value = attribute->getOpenGLType() == GL_INT
                           ? attribute->getInt(i)
                           : attribute->getFloat(i);

        // calculate the color based on the value of the attribute by using
        // the colorizer we got passed from the UI
        Color color = colorizer->getColor(value);

        attributeColor->setFloatComponent(i, 0, color.red());
        attributeColor->setFloatComponent(i, 1, color.green());
        attributeColor->setFloatComponent(i, 2, color.blue());
        attributeColor->setFloatComponent(i, 3, color.alpha());
        attributeColorBackside->setFloatComponent(i, 0, color.red());
        attributeColorBackside->setFloatComponent(i, 1, color.green());
        attributeColorBackside->setFloatComponent(i, 2, color.blue());
        attributeColorBackside->setFloatComponent(i, 3, color.alpha());
      }

      Q_EMIT colorizationDone(newSurface, operation);
    } else {
      qWarning() << "Input dataset for surface colorization is not a "
                    "VolumeDataVoxel";
      Q_EMIT colorizationDone(QSharedPointer<SurfaceDataTriangleIndexed>(),
                              operation);
    }
  } catch (OperationCancelledException&) {
    Q_EMIT colorizationDone(QSharedPointer<SurfaceDataTriangleIndexed>(),
                            operation);
  }
}
