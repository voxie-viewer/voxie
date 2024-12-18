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

#include "ParameterCopyBase.hpp"

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieBackend/Data/Data.hpp>

#include <QtCore/QSet>

using namespace vx;

ParameterCopyBase::ParameterCopyBase(
    const QDBusObjectPath& mainNodePath,
    const QMap<QDBusObjectPath, QSharedPointer<const QMap<QString, QVariant>>>&
        properties,
    const QMap<QDBusObjectPath, QSharedPointer<NodePrototype>>& prototypes,
    const QMap<QDBusObjectPath, QMap<QString, QString>>& extensionInfo)
    : mainNodePath_(mainNodePath),
      properties_(properties),
      prototypes_(prototypes),
      extensionInfo_(extensionInfo) {}

const QDBusObjectPath& ParameterCopyBase::mainNodePath() const {
  return mainNodePath_;
}

const QMap<QDBusObjectPath, QSharedPointer<const QMap<QString, QVariant>>>&
ParameterCopyBase::properties() const {
  return properties_;
}

const QMap<QDBusObjectPath, QSharedPointer<NodePrototype>>&
ParameterCopyBase::prototypes() const {
  return prototypes_;
}

const QMap<QDBusObjectPath, QMap<QString, QString>>&
ParameterCopyBase::extensionInfo() const {
  return extensionInfo_;
}
