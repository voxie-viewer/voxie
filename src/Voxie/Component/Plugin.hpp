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

//#include <plugin/interfaces.hpp>

#pragma once

#include <Voxie/Voxie.hpp>

#include <Voxie/Component/Interfaces.hpp>

#include <VoxieBackend/Component/ComponentContainer.hpp>

#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/ObjectExport/ExportedObject.hpp>

namespace vx {

class Plugin;

class VOXIECORESHARED_EXPORT PluginInstance : public QObject {
  Q_OBJECT

  friend class Plugin;

 public:
  PluginInstance();
  ~PluginInstance() override;

 protected:
  /**
   * This method will be called once by voxie to create all components provided
   * by the plugin.
   */
  virtual QList<QSharedPointer<vx::Component>> createComponents() = 0;
};

class VOXIECORESHARED_EXPORT Plugin : public vx::ComponentContainer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(Plugin)

 protected:
  void initialize() override;

 private:
  QObject* plugin;
  QString pluginName;
  QList<QSharedPointer<vx::Component>> allScriptExtensions;
  QList<QAction*> allUiCommands;
  QList<QSharedPointer<vx::plugin::MetaFilter2D>> allFilters2D;
  QList<QSharedPointer<vx::io::Importer>> allImporters;
  QList<QSharedPointer<vx::io::Exporter>> allExporters;
  QList<QSharedPointer<vx::io::SliceExporter>> allSliceExporters;

  QSharedPointer<const QList<QSharedPointer<ComponentType>>> componentTypes_;

 protected:  // TODO: should be private
  bool isCorePlugin_ = false;

 public:  // TODO: should not be public
  QList<QSharedPointer<vx::NodePrototype>> allObjectPrototypes;

 private:
  QMap<QString, QList<QSharedPointer<vx::plugin::Component>>> allObjects;
  QMap<QString, QMap<QString, QSharedPointer<vx::plugin::Component>>>
      allObjectsByName;

  template <typename ListT>
  void addObjects(const QString& name, const ListT& objects,
                  bool doRegisterChildren = true);

 public:  // TODO: should this be public?
  void reAddPrototypes();

 public:
  explicit Plugin(
      QObject* plugin, const QString& name,
      const QSharedPointer<const QList<QSharedPointer<ComponentType>>>&
          componentTypes);
  virtual ~Plugin();

  QString name() const { return this->pluginName; }

  // TODO: Remove these methods

  const QList<QSharedPointer<vx::plugin::Component>>& scriptExtensions() const {
    return this->allScriptExtensions;
  }

  const QList<QAction*>& uiCommands() const { return this->allUiCommands; }

 private:
  const QList<QSharedPointer<vx::plugin::MetaFilter2D>>& filters2D() const {
    return this->allFilters2D;
  }

  const QList<QSharedPointer<vx::io::Importer>>& importers() const {
    return this->allImporters;
  }

  const QList<QSharedPointer<vx::io::Exporter>>& exporters() const {
    return this->allExporters;
  }

  const QList<QSharedPointer<vx::io::SliceExporter>>& sliceExporters() const {
    return this->allSliceExporters;
  }

  const QList<QSharedPointer<vx::NodePrototype>>& objectPrototypes() const {
    return this->allObjectPrototypes;
  }

 public:
  QWidget* preferencesWidget() {
    vx::plugin::IPreferencesPlugin* preferencesPlugin =
        qobject_cast<vx::plugin::IPreferencesPlugin*>(this->plugin);
    if (preferencesPlugin) {
      return preferencesPlugin->preferencesWidget();
    } else {
      return nullptr;
    }
  }

  const QMap<QString, QList<QSharedPointer<vx::plugin::Component>>>&
  getAllObjects() {
    return allObjects;
  }
  const QMap<QString, QMap<QString, QSharedPointer<vx::plugin::Component>>>&
  getAllObjectsByName() {
    return allObjectsByName;
  }

  QList<QSharedPointer<vx::Component>> listComponents(
      const QSharedPointer<ComponentType>& componentType) override;

  QSharedPointer<vx::Component> getComponent(
      const QSharedPointer<ComponentType>& componentType, const QString& name,
      bool allowCompatibilityNames) override;

  bool isCorePlugin() const { return isCorePlugin_; }

  QSharedPointer<const QList<QSharedPointer<ComponentType>>> componentTypes()
      override {
    return componentTypes_;
  }
};

namespace plugin {
using Plugin = vx::Plugin;  // TODO: remove
}  // namespace plugin
}  // namespace vx
