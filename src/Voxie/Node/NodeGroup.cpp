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

#include "NodeGroup.hpp"

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Node/NodePrototype.hpp>

VX_NODE_INSTANTIATION(vx::NodeGroup)

using namespace vx;

NodeGroup::NodeGroup()
    : Node("NodeGroup", getPrototypeSingleton()),
      properties(new PropertiesType(this)) {
  setAutomaticDisplayName("NodeGroup");
}

NodeGroup::~NodeGroup() {
  for (QSharedPointer<Node> node : groupChildren()) {
    node->destroy();
  }
}

QList<QString> NodeGroup::supportedDBusInterfaces() { return {}; }

bool NodeGroup::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

bool NodeGroup::isAllowedParent(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

QList<QSharedPointer<Node>> NodeGroup::groupChildren(
    bool includeIndirectChildren) {
  // maybe save in a list instead of having to iterate over all nodes in the
  // project each time

  QList<QSharedPointer<Node>> nodes;

  for (auto node : vx::voxieRoot().nodes()) {
    if (node->parentNodeGroup().data() == this) {
      nodes.append(node);

      if (includeIndirectChildren) {
        NodeGroup* nodeGroup = dynamic_cast<NodeGroup*>(node.data());
        if (nodeGroup) {
          nodes.append(nodeGroup->groupChildren(true));
        }
      }
    }
  }

  return nodes;
}

QList<NodeNodeProperty> NodeGroup::childrenExportedProperties() {
  QList<NodeNodeProperty> result;
  for (auto node : groupChildren()) {
    for (auto property : node->exportedProperties()) {
      result.append(NodeNodeProperty(QPointer<Node>(node.data()), property));
    }
  }

  return result;
}
