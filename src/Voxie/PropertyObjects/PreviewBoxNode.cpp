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

#include "PreviewBoxNode.hpp"

#include <Voxie/Data/Prototypes.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <QObject>
#include <QPushButton>

using namespace vx;

VX_NODE_INSTANTIATION(vx::PreviewBoxNode)

PreviewBoxNode::PreviewBoxNode()
    : PropertyNode("PreviewBoxNode", getPrototypeSingleton()),
      properties(new PreviewBoxProperties(this)) {
  this->propertyName = "Preview Box";
  this->setAutomaticDisplayName(this->propertyName);

  PropertySection* section = new PropertySection();
  section->setWindowTitle(this->propertyName);
  this->addPropertySection(section);
  this->sizeWidget = new QuaternionWidget(new QLabel("New Size:"));
  this->originWidget = new QuaternionWidget(new QLabel("New Origin:"));
  this->rotationWidget = new QuaternionWidget(new QLabel("New Rotation:"));
  this->rotationWidget->setDisabled(true);
  this->activeCheckbox = new QCheckBox("active");
  section->addProperty(activeCheckbox);
  section->addProperty(sizeWidget);
  section->addProperty(originWidget);
  section->addProperty(rotationWidget);

  connect(this->originWidget, &QuaternionWidget::changed, this,
          &PreviewBoxNode::getPreviewBox);
  connect(this->sizeWidget, &QuaternionWidget::changed, this,
          &PreviewBoxNode::getPreviewBox);
  connect(this->rotationWidget, &QuaternionWidget::changed, this,
          &PreviewBoxNode::getPreviewBox);

  connect(this->activeCheckbox, &QCheckBox::stateChanged, this, [&](int i) {
    (void)i;
    this->previewBox->setActive(this->activeCheckbox->isChecked());
  });
}

PreviewBox* PreviewBoxNode::getPreviewBox() {
  QVector3D sizeData = QVector3D();
  if (!this->sizeWidget->allEmpty()) {
    sizeData = this->sizeWidget->getValues();
  }
  QVector3D originData = QVector3D();
  if (!this->sizeWidget->allEmpty()) {
    originData = this->originWidget->getValues();
  }
  this->previewBox->setOrigin(originData);
  this->previewBox->setSize(sizeData);
  this->previewBox->setActive(this->activeCheckbox->isChecked());
  return this->previewBox;
}
