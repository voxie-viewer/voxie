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

#include "GeometricPrimitiveObject.hpp"

#include <Voxie/Data/Prototypes.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Gui/PointList.hpp>

using namespace vx;

GeometricPrimitiveNode::GeometricPrimitiveNode()
    : DataNode("GeometricPrimitiveNode", getPrototypeSingleton()),
      properties(new GeometricPrimitiveProperties(this)),
      pointCounter(0) {
  this->setAutomaticDisplayName("Primitives");

  // TODO: should this automatically create a new data node or should data()
  // default to nullptr?
  setGeometricPrimitiveData(GeometricPrimitiveData::create());

  this->pointList = new PointList(this);

  pointList->setWindowTitle("Geometric Analysis");
  QObject::connect(this, &DataNode::dataChanged, pointList,
                   &PointList::reloadUIData);
  QObject::connect(properties,
                   &GeometricPrimitiveProperties::selectedPrimitiveChanged,
                   pointList, &PointList::reloadUIData);
  QObject::connect(properties,
                   &GeometricPrimitiveProperties::measurementPrimitive1Changed,
                   pointList, &PointList::reloadUIData);
  QObject::connect(properties,
                   &GeometricPrimitiveProperties::measurementPrimitive2Changed,
                   pointList, &PointList::reloadUIData);

  QObject::connect(properties,
                   &GeometricPrimitiveProperties::measurementPrimitive1Changed,
                   this, [this](quint64 one) {
                     Q_EMIT this->measurementPrimitivesChanged(
                         one, this->properties->measurementPrimitive2());
                   });
  QObject::connect(properties,
                   &GeometricPrimitiveProperties::measurementPrimitive2Changed,
                   this, [this](quint64 two) {
                     Q_EMIT this->measurementPrimitivesChanged(
                         this->properties->measurementPrimitive1(), two);
                   });
  QObject::connect(properties,
                   &GeometricPrimitiveProperties::selectedPrimitiveChanged,
                   this, &GeometricPrimitiveNode::selectedPrimitiveChanged);

  this->addPropertySection(pointList);
}
GeometricPrimitiveNode::~GeometricPrimitiveNode() {}

QSharedPointer<Data> GeometricPrimitiveNode::data() { return dataPointer; }

void GeometricPrimitiveNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<GeometricPrimitiveData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a GeometricPrimitiveData object");
  dataPointer = dataCast;
}

void GeometricPrimitiveNode::setGeometricPrimitiveData(
    const QSharedPointer<GeometricPrimitiveData>& data) {
  setData(data);
}

QString GeometricPrimitiveNode::nextPointName() {
  pointCounter++;
  return QString("New point #%1").arg(pointCounter);
}

std::tuple<bool, QVector3D, QVector3D>
GeometricPrimitiveNode::currentMeasurementPoints(
    const QSharedPointer<GeometricPrimitiveData>& gpd,
    GeometricPrimitivePropertiesBase* properties) {
  if (!gpd)
    return std::make_tuple(false, QVector3D(0, 0, 0), QVector3D(0, 0, 0));
  auto meas1 = properties->measurementPrimitive1();
  auto meas2 = properties->measurementPrimitive2();
  if (!meas1 || !meas2)
    return std::make_tuple(false, QVector3D(0, 0, 0), QVector3D(0, 0, 0));
  auto vec0 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      gpd->getPrimitiveOrNull(meas1));
  auto vec1 = qSharedPointerDynamicCast<GeometricPrimitivePoint>(
      gpd->getPrimitiveOrNull(meas2));
  if (!vec0 || !vec1)
    return std::make_tuple(false, QVector3D(0, 0, 0), QVector3D(0, 0, 0));
  return std::make_tuple(true, vec0->position(), vec1->position());
}
std::tuple<bool, QVector3D, QVector3D>
GeometricPrimitiveNode::currentMeasurementPoints() {
  return currentMeasurementPoints(this->geometricPrimitiveData(),
                                  this->properties);
}

NODE_PROTOTYPE_IMPL_DATA(GeometricPrimitive)
