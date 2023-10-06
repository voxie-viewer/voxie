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

#include "FitPlane.hpp"

#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/SurfaceNode.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Gui/PointList.hpp>

#include <PluginFitting/Prototypes.hpp>

#include <QtCore/QStringListModel>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>

#include <lapacke.h>

using namespace vx;
using namespace vx::vis3d;

FitPlane::FitPlane()
    : FilterNode(getPrototypeSingleton()),
      properties(new FitPlaneProperties(this)) {
  this->setAutomaticDisplayName("Fit plane to surface");
}

FitPlane::~FitPlane() {}

void FitPlane::fitSurface() {
  auto srfObj = dynamic_cast<SurfaceNode*>(this->properties->surface());
  // TODO: allow a vx::vis3d::Surface 3D node?
  if (!srfObj) {
    qWarning() << "No surface set";
    QMessageBox Msgbox(vx::voxieRoot().mainWindow());
    Msgbox.setText("No surface set.");
    Msgbox.exec();
    return;
  }
  auto srf =
      dynamic_cast<SurfaceDataTriangleIndexed*>(srfObj->surface().data());
  if (!srf) {
    qWarning() << "Not a SurfaceDataTriangleIndexed, ignoring";
    QMessageBox Msgbox(vx::voxieRoot().mainWindow());
    Msgbox.setText("Not a SurfaceDataTriangleIndexed, ignoring.");
    Msgbox.exec();
    return;
  }

  auto gpo = dynamic_cast<GeometricPrimitiveNode*>(
      this->properties->geometricPrimitive());
  if (!gpo) {
    qWarning() << "No geometric primitive node set";
    QMessageBox Msgbox(vx::voxieRoot().mainWindow());
    Msgbox.setText("No geometric primitive node set.");
    Msgbox.exec();
    return;
  }

  auto gpd = gpo->geometricPrimitiveData();
  if (!gpd) {
    qWarning() << "No geometric primitive data set";
    QMessageBox Msgbox(vx::voxieRoot().mainWindow());
    Msgbox.setText("No geometric primitive data set.");
    Msgbox.exec();
    return;
  }

  auto index1 = this->properties->point1();
  auto index2 = this->properties->point2();
  auto index3 = this->properties->point3();
  auto point1 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      gpd->getPrimitiveOrNull(index1));
  auto point2 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      gpd->getPrimitiveOrNull(index2));
  auto point3 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      gpd->getPrimitiveOrNull(index3));

  auto maximumDistance = this->properties->maximumDistance();

  // check if 3 selected points are unique
  if (point1 && point2 && point3 && index1 != index2 && index1 != index3 &&
      index2 != index3) {
    auto p1 = point1->position();
    auto p2 = point2->position();
    auto p3 = point3->position();

    // calculate a base plane to use in conjunction with threashold value
    // PlaneInfo planeBase = pointList->createNewPlaneFromPoints(*point0,
    // *point1, *point2);

    auto planeBase = PointList::createNewPlaneFromCoordinates(p1, p2, p3);

    QVector3D normal = planeBase.normal();
    auto p1n = normal + p1;
    auto p2n = normal + p3;

    // calculate 3 planes orthogonal to the base plate
    auto plane12 = PointList::createNewPlaneFromCoordinates(p1, p2, p1n);
    auto plane13 = PointList::createNewPlaneFromCoordinates(p1, p3, p1n);
    auto plane23 = PointList::createNewPlaneFromCoordinates(p2, p3, p2n);

    // find the relevant vertecies to use for the fitting
    float distance = plane12.distance(p3);
    int plane12sign = distance / std::abs(distance);
    distance = plane13.distance(p2);
    int plane13sign = distance / std::abs(distance);
    distance = plane23.distance(p1);
    int plane23sign = distance / std::abs(distance);
    std::vector<QVector3D> vertecies = std::vector<QVector3D>();
    for (QVector3D vertex : srf->vertices()) {
      if (std::abs(planeBase.distance(vertex)) < maximumDistance) {
        if (plane12.distance(vertex) * plane12sign > 0 &&
            plane13.distance(vertex) * plane13sign > 0 &&
            plane23.distance(vertex) * plane23sign > 0) {
          vertecies.push_back(vertex);
        }
      }
    }

    // form the data in Ax = b to solve with least squares
    int m = vertecies.size();
    int n = 3;
    std::vector<float> a(n * m);
    std::vector<float> b(m);
    float nrhs = 1;
    for (size_t k = 0; k < vertecies.size(); k++) {
      a[0 * m + k] = vertecies.at(k).x();
      a[1 * m + k] = vertecies.at(k).y();
      a[2 * m + k] = 1.0;
      b[k] = vertecies.at(k).z();
    }

    // calculate the coefficients
    LAPACKE_sgels(LAPACK_COL_MAJOR, 'N', m, n, nrhs, a.data(), m, b.data(), m);

    // find three new points and create a plane.
    p1.setZ(p1.x() * b[0] + p1.y() * b[1] + b[2]);
    p2.setZ(p2.x() * b[0] + p2.y() * b[1] + b[2]);
    p3.setZ(p3.x() * b[0] + p3.y() * b[1] + b[2]);
    PointList::newPlaneFromCoordinates(p1, p2, p3);

  } else {
    QMessageBox Msgbox(vx::voxieRoot().mainWindow());
    Msgbox.setText(
        "Please choose 3 different points to select a part of the surface "
        "for fitting.");
    Msgbox.exec();
  }
}

QSharedPointer<vx::io::RunFilterOperation> FitPlane::calculate(
    bool isAutomaticFilterRun) {
  Q_UNUSED(isAutomaticFilterRun);
  QSharedPointer<vx::io::RunFilterOperation> operation =
      vx::io::RunFilterOperation::createRunFilterOperation();
  // qDebug() << "FitPlane::calculate()";
  fitSurface();
  return operation;
}

NODE_PROTOTYPE_IMPL(FitPlane)
