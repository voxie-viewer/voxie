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

#include "ExtensionNodePrototype.hpp"

QSharedPointer<vx::NodePrototype> vx::parseNodePrototype(
    const QJsonObject& json) {
  auto prototypePtr = createQSharedPointer<QSharedPointer<NodePrototype>>();
  *prototypePtr = NodePrototype::fromJson(
      json, [prototypePtr](const QMap<QString, QVariant>& properties,
                           const QList<Node*>& inputs,
                           const QMap<QString, QDBusVariant>& options,
                           vx::NodePrototype* prototype, bool registerNode) {
        Q_UNUSED(prototype);  // Use *prototypePtr which is a shared ptr
        if (!*prototypePtr)
          throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                              "prototypePtr not set");

        auto ext =
            qSharedPointerDynamicCast<Extension>((*prototypePtr)->container());
        // Should always be in an extension
        if (!ext) {
          throw Exception("de.uni_stuttgart.Voxie.InternalError",
                          "extension is nullptr when creating project");
        }

        auto scriptFilename = ext->scriptFilename();

        auto node = QSharedPointer<vx::Node>();

        if ((*prototypePtr)->nodeKind() == NodeKind::Filter)
          node = ExtensionFilterNode::create(*prototypePtr, scriptFilename);
        else if ((*prototypePtr)->nodeKind() == NodeKind::SegmentationStep)
          node =
              ExtensionSegmentationStep::create(*prototypePtr, scriptFilename);
        else {
          throw Exception("de.uni_stuttgart.Voxie.InternalError",
                          "unknown node kind");
        }
        (*prototypePtr)
            ->createHelper(node, properties, inputs, options, registerNode);
        return node;
      });
  return *prototypePtr;
}

QSharedPointer<vx::Component> ComponentTypeInfoExt<NodePrototype>::parse(
    const QJsonObject& json,
    const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
        const QJsonObject&)>& parseProperties) {
  (void)parseProperties;
  return parseNodePrototype(json);
}
