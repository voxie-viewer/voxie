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

#include <Voxie/Component/MetaFilter2D.hpp>

#include <VoxieBackend/IO/Importer.hpp>

#include <VoxieBackend/Component/Component.hpp>

#include <QtCore/QList>
#include <QtCore/QObject>

#include <QtWidgets/QAction>

namespace vx {
class NodePrototype;
namespace io {
class Exporter;
}  // namespace io
namespace plugin {

// TODO: Remove most of these interfaces, use PluginInstance instead

class IPreferencesPlugin {
 public:
  virtual ~IPreferencesPlugin() {}
  virtual QWidget* preferencesWidget() = 0;
};

class IScriptExtensionPlugin {
 public:
  virtual ~IScriptExtensionPlugin() {}
  virtual QList<QSharedPointer<Component>> scriptExtensions() = 0;
};

class IFilter2DPlugin {
 public:
  virtual ~IFilter2DPlugin() {}
  virtual QList<QSharedPointer<MetaFilter2D>> filters2D() = 0;
};

class IImporterPlugin {
 public:
  virtual ~IImporterPlugin() {}
  virtual QList<QSharedPointer<vx::io::Importer>> importers() = 0;
};

class IExporterPlugin {
 public:
  virtual ~IExporterPlugin() {}
  virtual QList<QSharedPointer<vx::io::Exporter>> exporters() = 0;
};

class IObjectPrototypePlugin {
 public:
  virtual ~IObjectPrototypePlugin() {}
  virtual QList<QSharedPointer<vx::NodePrototype>> objectPrototypes() = 0;
};

}  // namespace plugin
}  // namespace vx

QT_BEGIN_NAMESPACE

#define V_DECLARE_INTERFACE(iface, name) \
  Q_DECLARE_INTERFACE(vx::plugin::iface, name)

V_DECLARE_INTERFACE(IPreferencesPlugin,
                    "de.uni_stuttgart.Voxie.Plugin.IPreferencesPlugin")
V_DECLARE_INTERFACE(IScriptExtensionPlugin,
                    "de.uni_stuttgart.Voxie.Plugin.IScriptExtensionPlugin")
V_DECLARE_INTERFACE(IFilter2DPlugin,
                    "de.uni_stuttgart.Voxie.Plugin.IFilter2DPlugin")
V_DECLARE_INTERFACE(IImporterPlugin,
                    "de.uni_stuttgart.Voxie.Plugin.IImporterPlugin")
V_DECLARE_INTERFACE(IExporterPlugin,
                    "de.uni_stuttgart.Voxie.Plugin.IExporterPlugin")
V_DECLARE_INTERFACE(IObjectPrototypePlugin,
                    "de.uni_stuttgart.Voxie.Plugin.IObjectPrototypePlugin")

QT_END_NAMESPACE
