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

#include "VolumeSeriesNode.hpp"

#include <Voxie/IVoxie.hpp>

#include <Voxie/Data/BoundingBox3D.hpp>
#include <Voxie/Data/Prototypes.hpp>

#include <VoxieBackend/Data/VolumeSeriesData.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

#include <Voxie/Gui/VolumeSeriesNodeView.hpp>

#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <VoxieClient/DBusAdaptors.hpp>

#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>

VX_NODE_INSTANTIATION(vx::VolumeSeriesNode)

using namespace vx;
using namespace vx::internal;

VolumeSeriesNode::VolumeSeriesNode()
    : MovableDataNode("VolumeSeriesNode", getPrototypeSingleton()),
      properties(new VolumeSeriesProperties(this)) {
  connect(this, &MovableDataNode::adjustedPositionChanged, this, [this]() {
    this->emitCustomPropertyChanged(this->properties->translationProperty());
  });
  connect(this, &MovableDataNode::adjustedRotationChanged, this, [this]() {
    this->emitCustomPropertyChanged(this->properties->rotationProperty());
  });

  // TODO: This should only be triggered if the bounding box actually changes
  QObject::connect(this, &DataNode::dataChangedFinished, this,
                   &MovableDataNode::boundingBoxObjectChanged);

  if (!voxieRoot().isHeadless())
    this->addPropertySection(new vx::gui::VolumeSeriesNodeView(this));
}

VolumeSeriesNode::~VolumeSeriesNode() {}

QSharedPointer<Data> VolumeSeriesNode::data() { return dataPointer; }
void VolumeSeriesNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<VolumeSeriesData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a VolumeSeriesData object");

  this->dataPointer = dataCast;
}

BoundingBox3D VolumeSeriesNode::boundingBoxObject() {
  auto data = volumeSeriesData();
  if (!data) return BoundingBox3D::empty();
  return BoundingBox3D(data->volumeOrigin(),
                       data->volumeOrigin() + data->volumeSize());
}
