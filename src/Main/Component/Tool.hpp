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

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <Voxie/Node/NodeKind.hpp>

#include <QtCore/QProcess>

namespace vx {
class Extension;
class Node;
class NodePrototype;
class NodePrototype;
template <typename T>
class SharedFunPtr;
class PropertyBase;

// TODO: Export this as interfaces over DBus?
class Tool : public Component {
  Q_OBJECT

  QString name_;
  QString displayName_;
  QString description_;

 public:
  Tool(const QJsonObject& json);
  ~Tool() override;

  static QSharedPointer<Tool> parse(const QJsonObject& json);

  QProcess* start(const QStringList& arguments = QStringList(),
                  QProcess* process = new QProcess());

  QList<QString> supportedComponentDBusInterfaces() override;

  virtual QString toolType() = 0;
  const QString& name() const { return name_; }
  const QString& displayName() const { return displayName_; }
  const QString& description() const { return description_; }
};

class ToolTargetNode : public vx::Tool {
  Q_OBJECT

  QList<QString> targetNodePrototypes_;
  QList<vx::NodeKind> targetNodeKinds_;

 public:
  ToolTargetNode(const QJsonObject& json);
  ~ToolTargetNode() override;

  QString toolType() override;
  const QList<QString>& targetNodePrototypes() const {
    return targetNodePrototypes_;
  }
  const QList<vx::NodeKind>& targetNodeKinds() const {
    return targetNodeKinds_;
  }

  bool matches(const QSharedPointer<NodePrototype>& prototype);

  QProcess* startNode(vx::Node* target, QProcess* process = new QProcess());
};

template <>
struct ComponentTypeInfo<Tool> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.Tool";
  }
  static const QList<std::tuple<QString, bool>> compatibilityNames() {
    return {
        std::make_tuple("de.uni_stuttgart.Voxie.Tool", true),
    };
  }
};
template <>
struct ComponentTypeInfoExt<Tool> {
  static const char* jsonName() { return "Tool"; }
  static QSharedPointer<vx::Component> parse(
      const QJsonObject& json,
      const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
          const QJsonObject&)>& parseProperties) {
    (void)parseProperties;
    return Tool::parse(json);
  }
};
}  // namespace vx
