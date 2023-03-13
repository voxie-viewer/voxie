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

#include "Tool.hpp"

#include <Voxie/Node/NodePrototype.hpp>

#include <VoxieBackend/Component/Extension.hpp>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

using namespace vx;

Tool::Tool(const QJsonObject& json)
    : Component(ComponentTypeInfo<Tool>::name(), json) {
  name_ = json["Name"].toString();
  displayName_ = json["DisplayName"].toString();
  description_ = json["Description"].toString();
}
Tool::~Tool() {}

QSharedPointer<Tool> Tool::parse(const QJsonObject& json) {
  QString type = json["ToolType"].toString();

  if (type == "TargetObject") {
    if (Node::showWarningOnOldObjectNames())
      qWarning() << "Got tool with 'TargetObject' type, should be renamed to "
                    "'TargetNode'";
    return makeSharedQObject<ToolTargetNode>(json);
  }
  if (type == "TargetNode") return makeSharedQObject<ToolTargetNode>(json);

  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Unknown tool type: '" + type + "'");
}

// TODO: Use code instead?
QProcess* Tool::start(const QStringList& arguments, QProcess* process) {
  auto ext = qSharedPointerDynamicCast<Extension>(this->container());
  // Should always be in an extension
  if (!ext) {
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "extension is nullptr in importData()");
  }

  QList<QString> args2;
  args2 << "--voxie-tool=" + name();
  args2.append(arguments);

  return ext->start("RunTool", args2, process);
}

QList<QString> Tool::supportedComponentDBusInterfaces() {
  // TODO: Export this as interfaces over DBus?
  return {};
}

ToolTargetNode::ToolTargetNode(const QJsonObject& json) : Tool(json) {
  for (const auto& name : json["TargetNodePrototypes"].toArray())
    targetNodePrototypes_ << name.toString();
  for (const auto& kind : json["TargetNodeKinds"].toArray())
    targetNodeKinds_ << vx::parseNodeKind(kind.toString());

  // TODO: remove
  if (json.contains("TargetObjectNames")) {
    if (Node::showWarningOnOldObjectNames())
      qWarning()
          << "Got tool with 'TargetObjectNames' property, should be renamed to "
             "'TargetNodePrototypes'";
    for (const auto& name : json["TargetObjectNames"].toArray())
      targetNodePrototypes_ << name.toString();
  }
  if (json.contains("TargetNodeNames")) {
    if (Node::showWarningOnOldObjectNames())
      qWarning()
          << "Got tool with 'TargetNodeNames' property, should be renamed to "
             "'TargetNodePrototypes'";
    for (const auto& name : json["TargetNodeNames"].toArray())
      targetNodePrototypes_ << name.toString();
  }
  if (json.contains("TargetObjectKinds")) {
    if (Node::showWarningOnOldObjectNames())
      qWarning()
          << "Got tool with 'TargetObjectKinds' property, should be renamed to "
             "'TargetNodeKinds'";
    for (const auto& kind : json["TargetObjectKinds"].toArray())
      targetNodeKinds_ << vx::parseNodeKind(kind.toString());
  }
}
ToolTargetNode::~ToolTargetNode() {}

QString ToolTargetNode::toolType() { return "TargetNode"; }

bool ToolTargetNode::matches(const QSharedPointer<NodePrototype>& prototype) {
  for (const auto& kind : targetNodeKinds()) {
    if (kind == prototype->nodeKind()) return true;
  }
  for (const auto& name : targetNodePrototypes()) {
    if (name == prototype->name()) return true;
  }
  return false;
}

QProcess* ToolTargetNode::startNode(Node* target, QProcess* process) {
  QStringList args;
  args << "--voxie-script-target-object=" + target->getPath().path();

  return start(args, process);
}
