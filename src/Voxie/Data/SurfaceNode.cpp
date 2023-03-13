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

#include "SurfaceNode.hpp"

#include <QDebug>
#include <QPushButton>

#include <QtWidgets/QAction>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <VoxieBackend/Data/SurfaceAttribute.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <Voxie/IO/SaveFileDialog.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Exception.hpp>

using namespace vx;

SurfaceNode::SurfaceNode()
    : PositionInterface("SurfaceNode", getPrototypeSingleton()),
      properties(new SurfaceProperties(this)) {
  connect(this, &PositionInterface::adjustedPositionChanged, this, [this]() {
    this->emitCustomPropertyChanged(this->properties->translationProperty());
  });
  connect(this, &PositionInterface::adjustedRotationChanged, this, [this]() {
    this->emitCustomPropertyChanged(this->properties->rotationProperty());
  });

  // Update properties whenever data changes.
  QObject::connect(this, &DataNode::dataChanged, this,
                   &SurfaceNode::updateProperties);

  if (!voxieRoot().isHeadless()) {
    QWidget* propertySection = new QWidget();

    propertySection->setMaximumHeight(400);
    QVBoxLayout* splitLayout = new QVBoxLayout();
    {
      this->numberOfVerticesLabel = new QLabel();
      this->numberOfTrianglesLabel = new QLabel();
      this->attributesLabel = new QLabel();

      QFormLayout* form = new QFormLayout();
      form->addRow("Vertices: ", this->numberOfVerticesLabel);
      form->addRow("Triangles: ", this->numberOfTrianglesLabel);
      form->addRow("Attributes: ", this->attributesLabel);
      splitLayout->addLayout(form);
    }
    propertySection->setLayout(splitLayout);
    this->addPropertySection(propertySection);
    this->connect(this, &Node::displayNameChanged, propertySection,
                  &QWidget::setWindowTitle);
  }

  this->setAutomaticDisplayName("Surface");
}

SurfaceNode::~SurfaceNode() {}

QSharedPointer<Data> SurfaceNode::data() { return surface(); }
void SurfaceNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<SurfaceData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a SurfaceData object");

  // qDebug() << "setSurface";
  this->surface_ = dataCast;
}

void SurfaceNode::setSurface(QSharedPointer<SurfaceData> newSurface) {
  setData(newSurface);
}

void SurfaceNode::updateProperties() {
  if (voxieRoot().isHeadless()) return;

  if (auto srf = qSharedPointerDynamicCast<SurfaceDataTriangleIndexed>(
          this->surface())) {
    this->numberOfVerticesLabel->setText(
        QString::number(srf->vertices().size()));
    this->numberOfTrianglesLabel->setText(
        QString::number(srf->triangles().size()));

    QStringList attributes;
    for (const auto& attribute : srf->listAttributes())
      attributes << attribute->displayName();
    attributes.sort();
    this->attributesLabel->setText(attributes.join(", "));
  } else {
    this->numberOfVerticesLabel->setText("");
    this->numberOfTrianglesLabel->setText("");
    this->attributesLabel->setText("");
  }
}

VolumeNode* SurfaceNode::getOriginatingVolumeNode() {
  return this->getOriginatingVolumeNode(this->parentNodes());
}

VolumeNode* SurfaceNode::getOriginatingVolumeNode(QList<Node*> parents) {
  for (auto parent : parents) {
    auto dataset = dynamic_cast<VolumeNode*>(parent);
    if (dataset) {
      return dataset;
    }
    dataset = getOriginatingVolumeNode(parent->parentNodes());
    if (dataset) {
      return dataset;
    }
  }
  return nullptr;
}

void SurfaceNode::adjustPosition(QVector3D position, bool isAbsolute) {
  PositionInterface::adjustPosition(position, isAbsolute);
  auto dataset = getOriginatingVolumeNode();
  if (dataset) {
    dataset->adjustPosition(this->getAdjustedPosition(), true);
  }
}

void SurfaceNode::adjustRotation(QQuaternion rotation, bool isAbsolute) {
  PositionInterface::adjustRotation(rotation, isAbsolute);
  auto dataset = getOriginatingVolumeNode();
  if (dataset) {
    dataset->adjustRotation(this->getAdjustedRotation(), true);
  }
}

NODE_PROTOTYPE_IMPL_2(Surface, Node)
