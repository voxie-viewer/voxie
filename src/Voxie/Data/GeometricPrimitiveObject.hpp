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

#include <Voxie/Voxie.hpp>

#include <Voxie/Data/Prototypes.forward.hpp>

#include <Voxie/Node/DataNode.hpp>

namespace vx {
class GeometricPrimitiveData;
class PointList;

class VOXIECORESHARED_EXPORT GeometricPrimitiveNode : public DataNode {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION("de.uni_stuttgart.Voxie.Data.GeometricPrimitive")

  friend class PointList;  // TODO: Make 'properties' public?

 private:
  QSharedPointer<GeometricPrimitiveData> dataPointer;

  PointList* pointList;

  // Used for nextPointName()
  quint64 pointCounter;

 public:
  explicit GeometricPrimitiveNode();
  ~GeometricPrimitiveNode();

  QSharedPointer<Data> data() override;

  void setGeometricPrimitiveData(
      const QSharedPointer<GeometricPrimitiveData>& data);
  const QSharedPointer<GeometricPrimitiveData>& geometricPrimitiveData() const {
    return dataPointer;
  }

  /**
   * Return a bool whether there is a current measurement the measurement
   * points.
   */
  std::tuple<bool, QVector3D, QVector3D> currentMeasurementPoints();
  static std::tuple<bool, QVector3D, QVector3D> currentMeasurementPoints(
      const QSharedPointer<GeometricPrimitiveData>& gpd,
      data_prop::GeometricPrimitivePropertiesBase* properties);

  /**
   * Returns a more-or-less unique name for a new point.
   */
  QString nextPointName();

 protected:
  void setDataImpl(const QSharedPointer<Data>& data) override;

 Q_SIGNALS:
  void measurementPrimitivesChanged(quint64 one, quint64 two);
  void selectedPrimitiveChanged(quint64 point);
};
}  // namespace vx
