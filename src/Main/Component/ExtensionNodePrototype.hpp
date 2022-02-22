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

#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include "ExtensionFilterNode.hpp"
#include "ExtensionSegmentationStep.hpp"

#include <Voxie/IO/RunFilterOperation.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <VoxieBackend/Component/Extension.hpp>

namespace vx {

QSharedPointer<vx::NodePrototype> parseNodePrototype(const QJsonObject& json);

template <>
struct ComponentTypeInfoExt<NodePrototype> {
  static const char* jsonName() { return "NodePrototype"; }
  // In .cpp to avoid ExtensionFilterNode.hpp include
  static QSharedPointer<vx::Component> parse(
      const QJsonObject& json,
      const vx::SharedFunPtr<QList<QSharedPointer<PropertyBase>>(
          const QJsonObject&)>& parseProperties);
  static QList<std::tuple<QString, bool>> compatibilityJsonNames() {
    return {
        std::make_tuple("Prototypes", true),
        std::make_tuple("ObjectPrototype", Node::showWarningOnOldObjectNames()),
    };
  }
};

}  // namespace vx
