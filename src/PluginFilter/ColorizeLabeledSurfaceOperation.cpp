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

#include "ColorizeLabeledSurfaceOperation.hpp"

#include <Voxie/Data/ColorizerEntry.hpp>
#include <Voxie/Data/SurfaceNode.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <VoxieBackend/Data/SurfaceAttribute.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <PluginFilter/CreateSurface/SurfaceExtractor.hpp>

#include <QApplication>
#include <QPointer>
#include <unordered_map>

using namespace vx;
using namespace vx::io;

ColorizeLabeledSurfaceOperation::ColorizeLabeledSurfaceOperation(
    const QSharedPointer<RunFilterOperation>& operation)
    : operation(operation) {}
ColorizeLabeledSurfaceOperation::~ColorizeLabeledSurfaceOperation() {}

void ColorizeLabeledSurfaceOperation::colorizeModel(
    SurfaceNode* surfaceObj, Colorizer* colorizer,
    QSharedPointer<TableData> tableData, int keyColumnIndex,
    int valueColumnIndex) {
  try {
    auto* surface =
        dynamic_cast<SurfaceDataTriangleIndexed*>(surfaceObj->surface().data());

    if (surface) {
      // create a copy of the verticies and triangles
      std::vector<SurfaceDataTriangleIndexed::Triangle> triangles(
          surface->triangles().size());
      for (size_t i = 0; i < triangles.size(); i++) {
        triangles[i] = surface->triangles()[i];
      }

      std::vector<QVector3D> vertices(surface->vertices().size());
      for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = surface->vertices()[i];
      }

      auto labelIds = surface->getAttributeOrNull(
          "de.uni_stuttgart.Voxie.SurfaceAttribute.LabelID");
      auto labelIdsBackside = surface->getAttributeOrNull(
          "de.uni_stuttgart.Voxie.SurfaceAttribute.LabelIDBackside");

      QList<std::tuple<
          QString, QString, quint64, std::tuple<QString, quint32, QString>,
          QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
          attributes;
      if (labelIds)
        attributes.push_back(std::make_tuple(
            "de.uni_stuttgart.Voxie.SurfaceAttribute.Color", labelIds->kind(),
            4, std::make_tuple("float", 32, "native"), "Color",
            QMap<QString, QDBusVariant>(), QMap<QString, QDBusVariant>()));
      if (labelIdsBackside)
        attributes.push_back(std::make_tuple(
            "de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside",
            labelIdsBackside->kind(), 4, std::make_tuple("float", 32, "native"),
            "Backside Color", QMap<QString, QDBusVariant>(),
            QMap<QString, QDBusVariant>()));

      auto newSurface = SurfaceDataTriangleIndexed::create(vertices, triangles,
                                                           false, attributes);

      QList<TableRow> rowData;
      if (tableData && tableData->columns().size() > keyColumnIndex &&
          keyColumnIndex >= 0 &&
          tableData->columns().size() > valueColumnIndex &&
          valueColumnIndex >= 0) {
        rowData = tableData->getRowsByIndex(0, tableData->rowCount());

        for (int i = 0; i < rowData.size(); i++) {
          auto key = rowData[i].data()[keyColumnIndex].value<int>();
          auto value = rowData[i].data()[valueColumnIndex].value<float>();
          rowMapping.emplace(key, value);
        }
      }

      // random values used to randomly colorize the labeled data
      QVector<quint32> randomValues(128);
      for (int i = 0; i < randomValues.size(); i++) {
        randomValues[i] = rand() % 128;
      }

      // colorize the outside vertices
      if (labelIds) {
        generateColorData(colorizer, labelIds, randomValues,
                          newSurface->getAttribute(
                              "de.uni_stuttgart.Voxie.SurfaceAttribute.Color"));
      }

      // colorize the inside vertices
      if (labelIdsBackside) {
        generateColorData(
            colorizer, labelIdsBackside, randomValues,
            newSurface->getAttribute(
                "de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside"));
      }

      // surface->moveToThread(QApplication::instance()->thread());
      Q_EMIT colorizationDone(newSurface, operation);
    } else {
      // TODO: Non-voxel datasets?
      qWarning()
          << "Input dataset for surface colorization is not a VolumeDataVoxel";
      Q_EMIT colorizationDone(QSharedPointer<SurfaceDataTriangleIndexed>(),
                              operation);
    }
  } catch (OperationCancelledException&) {
    Q_EMIT colorizationDone(QSharedPointer<SurfaceDataTriangleIndexed>(),
                            operation);
  }
}

void ColorizeLabeledSurfaceOperation::generateColorData(
    Colorizer* colorizer, QSharedPointer<SurfaceAttribute> labelIds,
    QVector<quint32> randomValues,
    const QSharedPointer<vx::SurfaceAttribute>& outputAttribute) {
  for (quint64 i = 0; i < labelIds->getSize(); i++) {
    auto labelId = labelIds->getInt(i);

    Color color;

    if (!rowMapping.empty()) {
      auto labelValue = 0.0f;
      labelValue = rowMapping[labelId];
      color = colorizer->getColor(labelValue);
    } else {
      // random value between 0 and 1
      auto randomValue =
          (randomValues[labelId % 128] + 64.0f) / (128.0f + 64.0f);
      color = colorizer->getColor(randomValue);
    }

    outputAttribute->setFloatComponent(i, 0, color.red());
    outputAttribute->setFloatComponent(i, 1, color.green());
    outputAttribute->setFloatComponent(i, 2, color.blue());
    outputAttribute->setFloatComponent(i, 3, color.alpha());
  }
}
