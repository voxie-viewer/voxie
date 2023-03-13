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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "VolumeNode.hpp"

#include <Voxie/IVoxie.hpp>

#include <Voxie/Data/BoundingBox3D.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/Slice.hpp>

#include <VoxieBackend/Data/VolumeData.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

#include <Voxie/Gui/VolumeNodeView.hpp>

#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>

using namespace vx;
using namespace vx::internal;

VolumeNode::VolumeNode()
    : PositionInterface("VolumeNode", getPrototypeSingleton()),
      properties(new VolumeProperties(this)) {
  connect(this, &PositionInterface::adjustedPositionChanged, this, [this]() {
    this->emitCustomPropertyChanged(this->properties->translationProperty());
  });
  connect(this, &PositionInterface::adjustedRotationChanged, this, [this]() {
    this->emitCustomPropertyChanged(this->properties->rotationProperty());
  });

  QObject::connect(properties, &VolumeProperties::rotationChanged, this,
                   &VolumeNode::rotationChanged);
  QObject::connect(properties, &VolumeProperties::translationChanged, this,
                   &VolumeNode::translationChanged);

  // TODO: This should be created with a nullptr volumeData, but there probably
  // are some null checks missing somewhere
  volumeDataPointer = VolumeDataVoxel::createVolume(1, 1, 1, DataType::Float32);

  if (!voxieRoot().isHeadless())
    this->addPropertySection(new vx::gui::VolumeNodeView(this));
}

VolumeNode::~VolumeNode() {}

QSharedPointer<Data> VolumeNode::data() { return volumeDataPointer; }
void VolumeNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<VolumeData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a VolumeData object");

  this->volumeDataPointer = dataCast;
  Q_EMIT changed();  // TODO: Is this needed?
}

void VolumeNode::setVolumeData(const QSharedPointer<VolumeData>& data) {
  setData(data);
}

// TODO: Shouldn't this consider properties->translation() /
// properties->rotationChanged()?
QVector3D VolumeNode::origin() const {
  auto data = this->volumeData();
  if (data)
    return data->origin();
  else
    return QVector3D(0, 0, 0);
}

QQuaternion VolumeNode::orientation() const {
  return this->properties->rotation();
}

AffineMap<double, 3UL, 3UL> VolumeNode::getObjectToGlobalTrafo() {
  auto trans = mapCast<double>(createTranslation(toVector(origin())));
  AffineMap<double, 3, 3> rot = rotationCast<double>(toRotation(orientation()));

  return trans * rot;
}

// TODO: Shouldn't this consider properties->translation() /
// properties->rotationChanged()?
QVector3D VolumeNode::size() const {
  auto data = this->volumeData();
  if (data)
    return data->getDimensionsMetric();
  else
    return QVector3D(0, 0, 0);
}

float VolumeNode::diagonalSize() const { return size().length(); }

// TODO: Shouldn't this consider properties->translation() /
// properties->rotationChanged()?
BoundingBox3D VolumeNode::boundingBox() {
  // TODO: consider adjustedPosition and adjustedRotation
  return BoundingBox3D::point(origin()) +
         BoundingBox3D::point(origin() + size());
}

/*
float VolumeNode::voxelSize() const {
  QVector3D datasetXYZDimInMeter = this->size();
  double datasetVolumen = datasetXYZDimInMeter.x() * datasetXYZDimInMeter.y() *
                          datasetXYZDimInMeter.z();
  int voxelCount = this->voxelCount();
  double voxelVolumen = datasetVolumen / voxelCount;
  return pow(voxelVolumen, 1.0 / 3.0);
}
*/

// TODO: Shouldn't this consider properties->translation() /
// properties->rotationChanged()?
QVector3D VolumeNode::volumeCenter() const { return origin() + size() / 2; }

NODE_PROTOTYPE_IMPL_2(Volume, Node)
