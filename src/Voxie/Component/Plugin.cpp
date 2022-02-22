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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "Plugin.hpp"

#include <VoxieBackend/Component/ComponentType.hpp>

#include <VoxieBackend/IO/Exporter.hpp>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Exception.hpp>

using namespace vx;
using namespace vx::plugin;

namespace vx {
namespace {
class PluginAdaptorImpl : public PluginAdaptor {
  Plugin* object;

 public:
  PluginAdaptorImpl(Plugin* object) : PluginAdaptor(object), object(object) {}
  ~PluginAdaptorImpl() override {}

  bool isCorePlugin() const override { return object->isCorePlugin(); };

  QString name() const override { return object->name(); }
};

class DynamicObjectAdaptorImplPlugin : public DynamicObjectAdaptor {
  Plugin* object;

 public:
  DynamicObjectAdaptorImplPlugin(Plugin* object)
      : DynamicObjectAdaptor(object), object(object) {}
  ~DynamicObjectAdaptorImplPlugin() override {}

  QStringList supportedInterfaces() const override {
    try {
      // return object->supportedDBusInterfaces();
      return {"de.uni_stuttgart.Voxie.Plugin"};
    } catch (Exception& e) {
      e.handle(object);
      return {};
    }
  }
};
}  // namespace
}  // namespace vx

template <typename T>
static inline T* getPtr(T* ptr) {
  return ptr;
}
template <typename T>
static inline T* getPtr(QSharedPointer<T> ptr) {
  return ptr.data();
}

template <typename T>
static inline bool isSharedPtr(T* ptr) {
  Q_UNUSED(ptr);
  return false;
}
template <typename T>
static inline bool isSharedPtr(QSharedPointer<T> ptr) {
  Q_UNUSED(ptr);
  return true;
}

// TODO: Remove this
template <typename ListT>
static inline void registerChildren(Plugin* plugin, const ListT& children,
                                    const QString& type) {
  QObject* container = new QObject(plugin);
  container->setObjectName(type);
  for (auto child : children) {
    if (child->objectName() == "") {
      if (auto prototype = dynamic_cast<vx::NodePrototype*>(getPtr(child)))
        child->setObjectName(prototype->name());
      else
        child->setObjectName(child->metaObject()->className());
    }
    if (!isSharedPtr(child)) child->setParent(container);
  }
}

PluginInstance::PluginInstance() {}
PluginInstance::~PluginInstance() {}

template <typename ListT>
void Plugin::addObjects(const QString& typeShort, const ListT& objects,
                        bool doRegisterChildren) {
  if (doRegisterChildren) registerChildren(this, objects, typeShort);

  for (const auto& child : objects)
    if (child->objectName() == "")
      if (auto prototype = dynamic_cast<vx::NodePrototype*>(getPtr(child)))
        child->setObjectName(prototype->name());

  QString type = "de.uni_stuttgart.Voxie.ComponentType." + typeShort;

  allObjects[type] = QList<QSharedPointer<Component>>();
  auto& objectsVector = allObjects[type];

  allObjectsByName[type] = QMap<QString, QSharedPointer<Component>>();
  auto& objectsMap = allObjectsByName[type];

  for (auto object : objects) {
    if (object->componentType() != type) {
      qWarning() << "Expected component type" << type << "for component"
                 << object->name() << "got" << object->componentType();
    }
    this->setContainer(object);
    objectsVector.push_back(object);
    if (objectsMap.find(object->name()) != objectsMap.end()) {
      qWarning() << "Warning: Got" << typeShort << "with name" << object->name()
                 << "twice in plugin" << this->name();
    } else {
      objectsMap[object->name()] = object;
    }
  }
}

void Plugin::reAddPrototypes() {
  addObjects("NodePrototype", allObjectPrototypes, false);
}

Plugin::Plugin(QObject* plugin, const QString& name,
               const QSharedPointer<const QList<QSharedPointer<ComponentType>>>&
                   componentTypes)
    : ComponentContainer("Plugin"),
      plugin(plugin),
      pluginName("unknown"),
      allScriptExtensions(),
      allUiCommands(),
      allFilters2D(),
      allImporters(),
      allExporters(),
      componentTypes_(componentTypes) {
  new DynamicObjectAdaptorImplPlugin(this);
  new PluginAdaptorImpl(this);

  this->pluginName = name;

  // Register in script interface
  this->setObjectName(this->pluginName);
}

void Plugin::initialize() {
  ComponentContainer::initialize();

  IScriptExtensionPlugin* scriptExtensionPlugin =
      qobject_cast<IScriptExtensionPlugin*>(plugin);
  if (scriptExtensionPlugin) {
    this->allScriptExtensions = scriptExtensionPlugin->scriptExtensions();
  }

  IUICommandPlugin* uiCommandPlugin = qobject_cast<IUICommandPlugin*>(plugin);
  if (uiCommandPlugin) {
    this->allUiCommands = uiCommandPlugin->uiCommands();
  }

  IFilter2DPlugin* filter2dPlugin = qobject_cast<IFilter2DPlugin*>(plugin);
  if (filter2dPlugin) {
    this->allFilters2D = filter2dPlugin->filters2D();
  }

  IImporterPlugin* importerPlugin = qobject_cast<IImporterPlugin*>(plugin);
  if (importerPlugin) {
    this->allImporters = importerPlugin->importers();
  }

  IExporterPlugin* exporterPlugin = qobject_cast<IExporterPlugin*>(plugin);
  if (exporterPlugin) {
    this->allExporters = exporterPlugin->exporters();
  }

  ISliceExportPlugin* sliceExportPlugin =
      qobject_cast<ISliceExportPlugin*>(plugin);
  if (sliceExportPlugin) {
    this->allSliceExporters = sliceExportPlugin->sliceExporters();
  }

  auto objectPrototypePlugin = qobject_cast<IObjectPrototypePlugin*>(plugin);
  if (objectPrototypePlugin)
    this->allObjectPrototypes = objectPrototypePlugin->objectPrototypes();

  addObjects("ScriptExtension", this->allScriptExtensions);
  // Set up parent of UI commands (so they will be when the Plugin is
  // deleted) but do not make them available over ListMembers() /
  // GetMemberByName() (because they are QActions and not Components)
  registerChildren(this, this->allUiCommands, "UICommands");
  addObjects("Filter2DPrototype", this->allFilters2D);
  addObjects("Importer", this->allImporters);
  addObjects("Exporter", this->allExporters);
  addObjects("SliceExporter", this->allSliceExporters);
  addObjects("NodePrototype", this->allObjectPrototypes);
  addObjects("Tool", QList<QSharedPointer<Component>>());
  // TODO: Support these as plugin members?
  addObjects("PropertyType", QList<QSharedPointer<Component>>());
  addObjects("GeometricPrimitiveType", QList<QSharedPointer<Component>>());

  if (auto pluginInstance = dynamic_cast<PluginInstance*>(plugin)) {
    try {
      auto components = pluginInstance->createComponents();

      for (const auto& component : components) {
        auto type = component->componentType();

        // TODO: Is this needed?
        if (component->objectName() == "") {
          if (auto prototype =
                  dynamic_cast<vx::NodePrototype*>(component.data()))
            component->setObjectName(prototype->name());
          else
            component->setObjectName(component->metaObject()->className());
        }

        if (!allObjects.contains(type) || !allObjectsByName.contains(type)) {
          qWarning() << "Encountered unknown component type" << type
                     << "in plugin" << this->name();
          continue;
        }

        auto& objectsVector = allObjects[type];
        auto& objectsMap = allObjectsByName[type];

        this->setContainer(component);
        objectsVector.push_back(component);
        if (objectsMap.find(component->name()) != objectsMap.end()) {
          qWarning() << "Warning: Got" << type << "with name"
                     << component->name() << "twice in plugin" << this->name();
        } else {
          objectsMap[component->name()] = component;
        }
      }
    } catch (vx::Exception& e) {
      qWarning() << "Exception while initializing plugin" << this->name() << ":"
                 << e.what();
    }
  } else {
    // TODO: Enable this warning once all plugins implement PluginInstance
    // qWarning() << "Plugin instance for plugin" << this->name()
    //            << "does not implement PluginInstance";
  }
}

Plugin::~Plugin() {}

QList<QSharedPointer<Component>> Plugin::listComponents(
    const QSharedPointer<ComponentType>& componentType) {
  const auto& allObjects = getAllObjects();
  auto it = allObjects.find(componentType->name());
  if (it == allObjects.end())
    throw Exception("de.uni_stuttgart.Voxie.InvalidComponentType",
                    "Unknown plugin member type: " + componentType->name());
  return *it;
}

QSharedPointer<Component> Plugin::getComponent(
    const QSharedPointer<ComponentType>& componentType, const QString& name,
    bool allowCompatibilityNames) {
  // There currently are no compatibility names in plugins
  (void)allowCompatibilityNames;

  const auto& allObjectsByName = getAllObjectsByName();
  if (allObjectsByName.find(componentType->name()) == allObjectsByName.end())
    throw Exception("de.uni_stuttgart.Voxie.InvalidComponentType",
                    "Unknown plugin member type: " + componentType->name());
  const auto& map = allObjectsByName[componentType->name()];

  if (map.find(name) == map.end())
    throw Exception("de.uni_stuttgart.Voxie.ComponentNotFound",
                    "Could not find component '" + name + "' with type '" +
                        componentType->name() + "'");

  return map[name];
}
