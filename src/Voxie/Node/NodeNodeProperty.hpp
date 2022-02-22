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

#include <QSharedPointer>
#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeProperty.hpp>

using namespace vx;

/***
 * @brief The "NodeProperty" class only holds a property of a node prototype,
 * but not what specific node it belongs to. This makes it impossible to get the
 * "value" of a property without knowing the corresponding node it belongs to.
 * This helper struct can be used to conveniently save a NodeProperty and the
 * Node it belongs to in one place.
 */
struct NodeNodeProperty {
  QPointer<Node> node;
  QSharedPointer<NodeProperty> property;

  NodeNodeProperty() {
    node = nullptr;
    property = QSharedPointer<NodeProperty>();
  }

  bool operator==(const NodeNodeProperty& other) const {
    return node == other.node && property == other.property;
  }

  NodeNodeProperty(QPointer<Node> _node,
                   QSharedPointer<NodeProperty> _property) {
    node = _node;
    property = _property;
  }
};
