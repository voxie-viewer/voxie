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

#include "NodeKind.hpp"

#include <VoxieClient/Exception.hpp>

using namespace vx;

NodeKind vx::parseNodeKind(const QString& kindStr) {
  if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.Data")
    return NodeKind::Data;
  else if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.Filter")
    return NodeKind::Filter;
  else if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.Property")
    return NodeKind::Property;
  else if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.Visualizer")
    return NodeKind::Visualizer;
  else if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.Object3D")
    return NodeKind::Object3D;
  else if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.SegmentationStep")
    return NodeKind::SegmentationStep;
  else if (kindStr == "de.uni_stuttgart.Voxie.NodeKind.NodeGroup")
    return NodeKind::NodeGroup;
  else
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidNodeKind",
                        "Unknown node kind");
}

QString vx::nodeKindToString(NodeKind kind) {
  switch (kind) {
    case NodeKind::Data:
      return "de.uni_stuttgart.Voxie.NodeKind.Data";
      break;
    case NodeKind::Filter:
      return "de.uni_stuttgart.Voxie.NodeKind.Filter";
      break;
    case NodeKind::Property:
      return "de.uni_stuttgart.Voxie.NodeKind.Property";
      break;
    case NodeKind::Visualizer:
      return "de.uni_stuttgart.Voxie.NodeKind.Visualizer";
      break;
    case NodeKind::Object3D:
      return "de.uni_stuttgart.Voxie.NodeKind.Object3D";
      break;
    case NodeKind::SegmentationStep:
      return "de.uni_stuttgart.Voxie.NodeKind.SegmentationStep";
      break;
    case NodeKind::NodeGroup:
      return "de.uni_stuttgart.Voxie.NodeKind.NodeGroup";
      break;
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Unknown node kind");
  }
}

NodeKind vx::parseObjectKind(const QString& kindStr) {
  if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.Data")
    return NodeKind::Data;
  else if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.Filter")
    return NodeKind::Filter;
  else if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.Property")
    return NodeKind::Property;
  else if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.Visualizer")
    return NodeKind::Visualizer;
  else if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.Object3D")
    return NodeKind::Object3D;
  else if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.SegmentationStep")
    return NodeKind::SegmentationStep;
  else if (kindStr == "de.uni_stuttgart.Voxie.ObjectKind.NodeGroup")
    return NodeKind::NodeGroup;
  else
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidObjectKind",
                        "Unknown object kind");
}

QString vx::objectKindToString(NodeKind kind) {
  switch (kind) {
    case NodeKind::Data:
      return "de.uni_stuttgart.Voxie.ObjectKind.Data";
      break;
    case NodeKind::Filter:
      return "de.uni_stuttgart.Voxie.ObjectKind.Filter";
      break;
    case NodeKind::Property:
      return "de.uni_stuttgart.Voxie.ObjectKind.Property";
      break;
    case NodeKind::Visualizer:
      return "de.uni_stuttgart.Voxie.ObjectKind.Visualizer";
      break;
    case NodeKind::Object3D:
      return "de.uni_stuttgart.Voxie.ObjectKind.Object3D";
      break;
    case NodeKind::SegmentationStep:
      return "de.uni_stuttgart.Voxie.ObjectKind.SegmentationStep";
      break;
    case NodeKind::NodeGroup:
      return "de.uni_stuttgart.Voxie.ObjectKind.NodeGroup";
      break;
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Unknown object kind");
  }
}

QList<NodeKind> vx::allNodeKinds() {
  return QList<NodeKind>{NodeKind::Data,     NodeKind::Filter,
                         NodeKind::Property, NodeKind::Visualizer,
                         NodeKind::Object3D, NodeKind::SegmentationStep,
                         NodeKind::NodeGroup};
}
