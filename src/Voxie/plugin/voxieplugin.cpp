#include "voxieplugin.hpp"

#include <Voxie/scripting/scriptingexception.hpp>

using namespace voxie::plugin;
using namespace voxie::plugin::internal;
using namespace voxie::scripting;

template<typename T>
static inline void registerChildren(VoxiePlugin *plugin, const QVector<T*> &children, const QString &type)
{
	QObject *container = new QObject(plugin);
	container->setObjectName(type);
	for(QObject *child : children) {
        if (child->objectName() == "")
            child->setObjectName(child->metaObject()->className());
		child->setParent(container);
	}
}

template <typename T> void VoxiePlugin::addObjects (const QString& typeShort, const QVector<T*>& objects) {
    registerChildren (this, objects, typeShort);

    QString type = "de.uni_stuttgart.Voxie." + typeShort;

    allObjects[type] = QVector<voxie::plugin::PluginMember*> ();
    auto& objectsVector = allObjects[type];
    
    allObjectsByName[type] = QMap<QString, voxie::plugin::PluginMember*> ();
    auto& objectsMap = allObjectsByName[type];

	for (PluginMember* object : objects) {
        if (object->plugin_) {
            qWarning() << "Warning: For plugin" << this->name() << typeShort << "member" << object->objectName() << ": object->plugin_ is already set";
        } else {
            object->plugin_ = this;
        }
        if (object->type_ != "") {
            qWarning() << "Warning: For plugin" << this->name() << typeShort << "member" << object->objectName() << ": object->type_ is already set";
        } else {
            object->type_ = type;
        }
        if (object->name_ != "") {
            qWarning() << "Warning: For plugin" << this->name() << typeShort << "member" << object->objectName() << ": object->name_ is already set";
        } else {
            object->name_ = object->objectName();
        }
        objectsVector.push_back (object);
        if (objectsMap.find (object->objectName ()) != objectsMap.end ()) {
            qWarning () << "Warning: Got" << typeShort << "with name" << object->objectName () << "twice in plugin" << this->name ();
        } else {
            objectsMap[object->objectName ()] = object;
        }
    }
}

VoxiePlugin::VoxiePlugin(QObject *plugin, QObject *parent) :
	ScriptableObject("Plugin", parent),
	plugin(plugin),
	pluginName("unknown"),
	allVisualizers(),
	allScriptExtensions(),
	allUiCommands(),
	allFilters2D(),
	allFilters3D(),
	allLoaders(),
	allImporters()
{
    new VoxiePluginAdaptor (this);

	if(plugin != nullptr)
	{
		this->pluginName = plugin->objectName();
		if(this->pluginName.length() == 0)
		{
			this->pluginName = plugin->metaObject()->className();
		}
	}

	// Register in script interface
	this->setObjectName(this->pluginName);

	IVisualizerPlugin *visualizerPlugin = qobject_cast<IVisualizerPlugin*>(plugin);
	if(visualizerPlugin)
	{
		this->allVisualizers = visualizerPlugin->visualizers();
	}

	IScriptExtensionPlugin *scriptExtensionPlugin = qobject_cast<IScriptExtensionPlugin*>(plugin);
	if(scriptExtensionPlugin)
	{
		this->allScriptExtensions = scriptExtensionPlugin->scriptExtensions();
	}

	IUICommandPlugin *uiCommandPlugin = qobject_cast<IUICommandPlugin*>(plugin);
	if(uiCommandPlugin)
	{
		this->allUiCommands = uiCommandPlugin->uiCommands();
	}

	IFilter2DPlugin *filter2dPlugin = qobject_cast<IFilter2DPlugin*>(plugin);
	if(filter2dPlugin)
	{
		this->allFilters2D = filter2dPlugin->filters2D();
	}

	IFilter3DPlugin *filter3DPlugin = qobject_cast<IFilter3DPlugin*>(plugin);
	if(filter3DPlugin)
	{
		this->allFilters3D = filter3DPlugin->filters3D();
	}

	ILoaderPlugin *loaderPlugin = qobject_cast<ILoaderPlugin*>(plugin);
	if(loaderPlugin)
	{
		this->allLoaders = loaderPlugin->loaders();
	}

	IImporterPlugin *importerPlugin = qobject_cast<IImporterPlugin*>(plugin);
	if(importerPlugin)
	{
		this->allImporters = importerPlugin->importers();
	}

	IVoxelExportPlugin *voxelExportPlugin = qobject_cast<IVoxelExportPlugin*>(plugin);
	if(voxelExportPlugin)
	{
		this->allVoxelExporters = voxelExportPlugin->voxelExporters();
	}

	ISliceExportPlugin *sliceExportPlugin = qobject_cast<ISliceExportPlugin*>(plugin);
	if(sliceExportPlugin)
	{
		this->allSliceExporters = sliceExportPlugin->sliceExporters();
	}

	addObjects("VisualizerFactory", this->allVisualizers);
	addObjects("ScriptExtension", this->allScriptExtensions);
	// Set up parent of UI commands (so they will be when the VoxiePlugin is
    // deleted) but do not make them available over ListMembers() /
    // GetMemberByName() (because they are QActions and not PluginMembers)
    registerChildren(this, this->allUiCommands, "UICommands");
	addObjects("Filter2DFactory", this->allFilters2D);
	addObjects("Filter3DFactory", this->allFilters3D);
	addObjects("Loader", this->allLoaders);
	addObjects("Importer", this->allImporters);
	addObjects("VoxelExporter", this->allVoxelExporters);
	addObjects("SliceExporter", this->allSliceExporters);
}

VoxiePlugin::~VoxiePlugin() {
}

PluginMember* VoxiePlugin::getMemberByName (const QString& type, const QString& name) {
    const auto& allObjectsByName = getAllObjectsByName();
    if (allObjectsByName.find (type) == allObjectsByName.end ())
        throw ScriptingException("de.uni_stuttgart.Voxie.InvalidPluginMemberType", "Unknown plugin member type");
    const auto& map = allObjectsByName[type];

    if (map.find (name) == map.end ())
        throw ScriptingException("de.uni_stuttgart.Voxie.PluginMemberNotFound", "Could not find plugin member");

    return map[name];
}

VoxiePluginAdaptor::VoxiePluginAdaptor (VoxiePlugin* object) : QDBusAbstractAdaptor (object), object (object) {
}
VoxiePluginAdaptor::~VoxiePluginAdaptor () {
}

QString VoxiePluginAdaptor::name() {
    return object->name();
}

QVector<QDBusObjectPath> VoxiePluginAdaptor::ListMembers (const QString& type) {
    try {
        const auto& allObjects = object->getAllObjects();
        if (allObjects.find (type) == allObjects.end ())
            throw ScriptingException("de.uni_stuttgart.Voxie.InvalidPluginMemberType", "Unknown plugin member type");

        QVector<QDBusObjectPath> objects;
        for (PluginMember* object : allObjects[type])
            objects.push_back (voxie::scripting::ScriptableObject::getPath(object));
        return objects;
    } catch (ScriptingException& e) {
        e.handle(object);
        return QVector<QDBusObjectPath> ();
    }
}

QDBusObjectPath VoxiePluginAdaptor::GetMemberByName (const QString& type, const QString& name) {
    try {
        return voxie::scripting::ScriptableObject::getPath (object->getMemberByName(type, name));
    } catch (ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptableObject::getPath(nullptr);
    }
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
