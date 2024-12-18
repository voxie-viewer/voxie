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

#include "TomographyRawDataNode.hpp"

#include <Voxie/Data/Prototypes.hpp>
#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>

#include <Voxie/Gui/TomographyRawDataNodeView.hpp>

#include <QtGui/QIcon>

VX_NODE_INSTANTIATION(vx::TomographyRawDataNode)

using namespace vx;

TomographyRawDataNode::TomographyRawDataNode()
    : DataNode("TomographyRawDataNode", getPrototypeSingleton()),
      properties(new PropertiesType(this)) {
  this->addPropertySection(new vx::gui::TomographyRawDataNodeView(this));
}
TomographyRawDataNode::~TomographyRawDataNode() {}

QSharedPointer<Data> TomographyRawDataNode::data() { return dataAccessor(); }

void TomographyRawDataNode::setDataImpl(const QSharedPointer<Data>& data) {
  auto dataCast = qSharedPointerDynamicCast<TomographyRawData2DAccessor>(data);
  if (!dataCast && data)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Not a TomographyRawData2DAccessor object");

  this->data_ = dataCast;
}
