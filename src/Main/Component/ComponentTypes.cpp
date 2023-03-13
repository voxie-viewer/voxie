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

#include "ComponentTypes.hpp"

#include <VoxieBackend/Component/ComponentType.hpp>
#include <VoxieBackend/Component/ExtensionExporter.hpp>
#include <VoxieBackend/Component/ExtensionImporter.hpp>

#include <VoxieBackend/Data/GeometricPrimitiveType.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <Voxie/Node/NodePrototype.hpp>

#include <Voxie/Component/MetaFilter2D.hpp>

#include <Main/Component/ExtensionNodePrototype.hpp>
#include <Main/Component/Tool.hpp>

using namespace vx;
using namespace vx::io;
using namespace vx::plugin;

QSharedPointer<const QList<QSharedPointer<ComponentType>>>
vx::getComponentTypes() {
  static QSharedPointer<const QList<QSharedPointer<ComponentType>>> types =
      createQSharedPointer<QList<QSharedPointer<ComponentType>>>(
          QList<QSharedPointer<ComponentType>>{
              createQSharedPointer<ComponentType>(
                  ComponentType::create<Exporter>()),
              createQSharedPointer<ComponentType>(
                  ComponentType::create<Importer>()),
              createQSharedPointer<ComponentType>(
                  ComponentType::create<NodePrototype>()),
              createQSharedPointer<ComponentType>(
                  ComponentType::create<Tool>()),
              createQSharedPointer<ComponentType>(
                  ComponentType::createNoExt<MetaFilter2D>()),
              createQSharedPointer<ComponentType>(
                  ComponentType::createNoExt<PropertyType>()),
              createQSharedPointer<ComponentType>(
                  ComponentType::createNoExt<GeometricPrimitiveType>()),
          });
  // TODO: ScriptExtension?
  return types;
}
