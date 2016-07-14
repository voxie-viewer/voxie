//#include <plugin/interfaces.hpp>

#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/plugin/interfaces.hpp>

#include <Voxie/scripting/dbustypes.hpp>
#include <Voxie/scripting/scriptingcontainer.hpp>

namespace voxie
{
namespace plugin
{

class VOXIECORESHARED_EXPORT VoxiePlugin :
        public voxie::scripting::ScriptableObject,
        public QDBusContext
{
    Q_OBJECT

private:
    QObject *plugin;
    QString pluginName;
    QVector<MetaVisualizer*> allVisualizers;
    QVector<voxie::plugin::PluginMember*> allScriptExtensions;
    QVector<QAction*> allUiCommands;
    QVector<MetaFilter2D*> allFilters2D;
    QVector<MetaFilter3D*> allFilters3D;
    QVector<voxie::io::Loader*> allLoaders;
    QVector<voxie::io::Importer*> allImporters;
    QVector<voxie::io::VoxelExporter*> allVoxelExporters;
    QVector<voxie::io::SliceExporter*> allSliceExporters;

    QMap<QString, QVector<voxie::plugin::PluginMember*>> allObjects;
    QMap<QString, QMap<QString, voxie::plugin::PluginMember*>> allObjectsByName;
    template <typename T> void addObjects (const QString& name, const QVector<T*>& objects);
public:
    explicit VoxiePlugin(QObject *plugin, QObject *parent = 0);
    virtual ~VoxiePlugin();

    QString name() const { return this->pluginName; }

    const QVector<MetaVisualizer*> &visualizers() const
    {
        return this->allVisualizers;
    }

    const QVector<voxie::plugin::PluginMember*> &scriptExtensions() const
    {
        return this->allScriptExtensions;
    }

    const QVector<QAction*> &uiCommands() const
    {
        return this->allUiCommands;
    }

    const QVector<MetaFilter2D*> &filters2D() const
    {
        return this->allFilters2D;
    }

    const QVector<MetaFilter3D*> &filters3D() const
    {
        return this->allFilters3D;
    }

    const QVector<voxie::io::Loader*> &loaders() const
    {
        return this->allLoaders;
    }

    const QVector<voxie::io::Importer*> &importers() const
    {
        return this->allImporters;
    }

    const QVector<voxie::io::VoxelExporter*> &voxelExporters() const
    {
        return this->allVoxelExporters;
    }

    const QVector<voxie::io::SliceExporter*> &sliceExporters() const
    {
        return this->allSliceExporters;
    }

    QWidget *preferencesWidget()
    {
        IPreferencesPlugin *preferencesPlugin = qobject_cast<IPreferencesPlugin*>(this->plugin);
        if(preferencesPlugin)
        {
            return preferencesPlugin->preferencesWidget();
        }
        else
        {
            return nullptr;
        }
    }

    const QMap<QString, QVector<voxie::plugin::PluginMember*>>& getAllObjects () { return allObjects; }
    const QMap<QString, QMap<QString, voxie::plugin::PluginMember*>>& getAllObjectsByName () { return allObjectsByName; }


    Q_INVOKABLE
    PluginMember* getMemberByName (const QString& type, const QString& name);

signals:

public slots:

};

namespace internal {
class VoxiePluginAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Plugin")

    VoxiePlugin* object;

public:
    VoxiePluginAdaptor (VoxiePlugin* object);
    virtual ~VoxiePluginAdaptor ();

    Q_PROPERTY (QString Name READ name)
    QString name();

public slots:
    QVector<QDBusObjectPath> ListMembers (const QString& type);
    QDBusObjectPath GetMemberByName (const QString& type, const QString& name);
};
}

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
