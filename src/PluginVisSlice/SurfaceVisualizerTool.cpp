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

#include "SurfaceVisualizerTool.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <VoxieBackend/Data/SurfaceData.hpp>

#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>

using namespace vx;

SurfaceVisualizerTool::SurfaceVisualizerTool(SliceVisualizer* sv) {
  color.setRgb(255, 0, 0);  // TODO: turn into property

  // Redraw when the slice changes
  QObject::connect(sv->properties, &SliceProperties::orientationChanged, this,
                   &Layer::triggerRedraw);
  QObject::connect(sv->properties, &SliceProperties::originChanged, this,
                   &Layer::triggerRedraw);

  // Redraw when shown part of slice changes
  connect(sv->properties, &SliceProperties::centerPointChanged, this,
          &Layer::triggerRedraw);
  connect(sv->properties, &SliceProperties::verticalSizeChanged, this,
          &Layer::triggerRedraw);

  // Redraw when surfaces are added / removed
  /*
  // Already covered by the forwarded SurfaceVisualizerTool::surfaceChanged
  connect(sv->properties, &SliceProperties::surfaceChanged, this,
          &Layer::triggerRedraw);
  */
  forwardSignalFromListPropertyNodeOnReconnect(
      sv->properties, &SliceProperties::surface,
      &SliceProperties::surfaceChanged, &DataNode::dataChanged, this,
      &SurfaceVisualizerTool::surfaceChanged);
  /*
  connect(this, &SurfaceVisualizerTool::surfaceChanged, this,
          []() { qDebug() << "SurfaceVisualizerTool::surfaceChanged()"; });
  */
  connect(this, &SurfaceVisualizerTool::surfaceChanged, this,
          &Layer::triggerRedraw);
}

void SurfaceVisualizerTool::render(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters) {
  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  PlaneInfo plane(properties.origin(), properties.orientation());
  auto planeArea =
      SliceVisualizer::getCurrentPlaneArea(&properties, outputImage.size());

  for (auto surfacePath : properties.surfaceRaw()) {
    SurfacePropertiesCopy surfaceProperties(
        parameters->properties()[surfacePath]);

    auto data = qSharedPointerDynamicCast<SurfaceData>(
        parameters->getData(surfacePath).data());
    if (!data) continue;

    auto surface = qSharedPointerDynamicCast<SurfaceDataTriangleIndexed>(data);
    if (!surface) {
      qWarning() << "Not a SurfaceDataTriangleIndexed, ignoring";
      continue;
    }

    // TODO: consider translation/rotation of surface

    for (SurfaceDataTriangleIndexed::Triangle triangle : surface->triangles()) {
      std::vector<SurfaceDataTriangleIndexed::IndexType> above;
      std::vector<float> aboveDistance;
      std::vector<SurfaceDataTriangleIndexed::IndexType> below;
      std::vector<float> belowDistance;
      std::vector<SurfaceDataTriangleIndexed::IndexType> same;
      // check if triangle is relevant
      for (short i = 0; i < 3; i++) {
        float distanceToPlane =
            plane.distance(surface->vertices()[triangle[i]]);
        if (distanceToPlane > 0) {
          aboveDistance.push_back(distanceToPlane);
          above.push_back(triangle[i]);
        } else if (distanceToPlane < 0) {
          belowDistance.push_back(distanceToPlane);
          below.push_back(triangle[i]);
        } else
          same.push_back(triangle[i]);
      }
      if (same.size() == 2 || (above.size() > 0 && below.size() > 0)) {
        // triangle is relevant. finding lines...
        if (same.size() == 2) {
          // both ends are on this zIndex
          QPointF p1 = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[same[0]]));
          QPointF p2 = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[same[1]]));
          drawLineOnImage(outputImage, p1, p2);
        } else if (same.size() == 1) {
          QPointF pAbove = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[above[0]]));
          QPointF pBelow = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[below[0]]));

          QPointF cutsPlaneAt;
          cutsPlaneAt.setX(pAbove.x() +
                           (pBelow.x() - pAbove.x()) * aboveDistance[0] /
                               (aboveDistance[0] - belowDistance[0]));
          cutsPlaneAt.setY(pAbove.y() +
                           (pBelow.y() - pAbove.y()) * aboveDistance[0] /
                               (aboveDistance[0] - belowDistance[0]));

          QPointF p1 = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[same[0]]));
          drawLineOnImage(outputImage, p1, cutsPlaneAt);
        } else {
          std::vector<SurfaceDataTriangleIndexed::IndexType>* list1 =
              above.size() == 1 ? &above : &below;
          std::vector<SurfaceDataTriangleIndexed::IndexType>* list2 =
              above.size() == 2 ? &above : &below;
          std::vector<float>* list1Distance =
              above.size() == 1 ? &aboveDistance : &belowDistance;
          std::vector<float>* list2Distance =
              above.size() == 2 ? &aboveDistance : &belowDistance;

          QPointF pList1 = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[list1->at(0)]));
          QPointF pList2 = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[list2->at(0)]));

          QPointF p1;
          p1.setX(pList1.x() +
                  (pList2.x() - pList1.x()) * list1Distance->at(0) /
                      (list1Distance->at(0) - list2Distance->at(0)));
          p1.setY(pList1.y() +
                  (pList2.y() - pList1.y()) * list1Distance->at(0) /
                      (list1Distance->at(0) - list2Distance->at(0)));

          pList2 = SliceVisualizer::planePointToPixel(
              outputImage.size(), planeArea,
              plane.get2DPlanePoint(surface->vertices()[list2->at(1)]));

          QPointF p2;
          p2.setX(pList1.x() +
                  (pList2.x() - pList1.x()) * list1Distance->at(0) /
                      (list1Distance->at(0) - list2Distance->at(1)));
          p2.setY(pList1.y() +
                  (pList2.y() - pList1.y()) * list1Distance->at(0) /
                      (list1Distance->at(0) - list2Distance->at(1)));
          drawLineOnImage(outputImage, p1, p2);
        }
      }
    }
  }
}

// TODO: reuse painter?
void SurfaceVisualizerTool::drawLineOnImage(QImage& img, QPointF p1,
                                            QPointF p2) {
  QPainter painter(&img);
  QPen pen;
  pen.setColor(color);
  painter.setPen(pen);
  painter.drawLine(p1, p2);
}
