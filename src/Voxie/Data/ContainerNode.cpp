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

#include "ContainerNode.hpp"
#include <QtCore/QFileInfo>
#include <QtWidgets/QLabel>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Voxie.hpp>

using namespace vx;

void ContainerNode::setCompoundPointer(
    QSharedPointer<ContainerData>& containerData) {
  this->containerDataPointer = containerData;
  setData(containerData);
}

ContainerNode::ContainerNode(QSharedPointer<ContainerData>& containerData)
    : DataNode("ContainerNode", getPrototypeSingleton()),
      properties(new ContainerProperties(this)) {
  setAutomaticDisplayName(tr("Container"));

  this->initializeInfoWidget();
  setCompoundPointer(containerData);

  connect(this, &QObject::destroyed, infoWidget, &QObject::deleteLater);
}

ContainerNode::ContainerNode()
    : DataNode("ContainerNode", getPrototypeSingleton()) {
  setAutomaticDisplayName(tr("Container"));
  this->initializeInfoWidget();
  connect(this, &QObject::destroyed, infoWidget, &QObject::deleteLater);
}

QSharedPointer<ContainerData> ContainerNode::getCompoundPointer() {
  return this->containerDataPointer;
}

void ContainerNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<ContainerData>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a ContainerData object");

  this->containerDataPointer = dataCast;
}

QSharedPointer<Data> ContainerNode::data() {
  return this->containerDataPointer;
}

QWidget* ContainerNode::getCustomPropertySectionContent(const QString& name) {
  if (name == "de.uni_stuttgart.Voxie.Data.Container.Info") {
    return this->infoWidget;
  } else {
    return {};
  }
}

void ContainerNode::initializeInfoWidget() {
  auto widget = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(layout);

  auto name = new QLabel("Name:");
  layout->addWidget(name);

  auto elementsLabel = new QLabel("Elements:");
  layout->addWidget(elementsLabel);

  connect(this, &DataNode::dataChanged, this, [=]() {
    if (this->containerDataPointer) {
      name->setText("Name: " + this->containerDataPointer->getName());

      QString elements = "Elements: \n";
      QList<QString> keys = this->containerDataPointer->getKeys();

      for (QString key : keys) {  //} i != keys.end(); ++i) {
        elements.append("\t" + key + "\n");
      }

      elementsLabel->setText(elements);
    }
  });

  this->infoWidget = widget;
}

NODE_PROTOTYPE_IMPL_2(Container, Node)
