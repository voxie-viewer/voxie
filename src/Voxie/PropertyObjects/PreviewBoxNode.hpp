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

#include <Voxie/Data/PreviewBox.hpp>
#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/Prototypes.forward.hpp>
#include <Voxie/Data/QuaternionWidget.hpp>

#include <Voxie/Node/PropertyNode.hpp>

#include <QCheckBox>

namespace vx {

/**
 * @brief The PreviewBoxNode class represents a node holding a reference to
 * a preview box. It can be connected to filter nodes to crop the data for
 * a faster processing
 */
class VOXIECORESHARED_EXPORT PreviewBoxNode : public PropertyNode {
  PreviewBoxProperties* properties;

  QuaternionWidget* originWidget;
  QuaternionWidget* sizeWidget;
  QuaternionWidget* rotationWidget;
  QCheckBox* activeCheckbox;

  PreviewBox* previewBox = new PreviewBox(QVector3D(), QVector3D(), false);

 public:
  static QSharedPointer<NodePrototype> getPrototypeSingleton();

  PreviewBoxNode();

  PreviewBox* getPreviewBox();
};
}  // namespace vx
