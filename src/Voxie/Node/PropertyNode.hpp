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

//#include <Voxie/Data/VolumeNode.hpp>
#include <Voxie/Node/Node.hpp>

#include <QString>

namespace vx {
class VOXIECORESHARED_EXPORT PropertyNode : public Node {
 public:
  /**
   * @brief The PropertyNode Class should be the base class of every Property
   * Node like Planes and PreviewBoxes.
   * @param type Name of the PropertyNode at DBus Interface
   */
  PropertyNode(const QString& type,
               const QSharedPointer<NodePrototype>& prototype);

  QList<QString> supportedDBusInterfaces() override;

  virtual bool isAllowedChild(NodeKind node) override;
  virtual bool isAllowedParent(NodeKind node) override;

  // VolumeNode* dataSet;
  QString propertyName;
};
}  // namespace vx
