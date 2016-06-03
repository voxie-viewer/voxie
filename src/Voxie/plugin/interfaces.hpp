#pragma once

#include <Voxie/io/importer.hpp>
#include <Voxie/io/loader.hpp>
#include <Voxie/io/sliceexporter.hpp>
#include <Voxie/io/voxelexporter.hpp>

#include <Voxie/plugin/metafilter2d.hpp>
#include <Voxie/plugin/metafilter3d.hpp>
#include <Voxie/plugin/metavisualizer.hpp>
#include <Voxie/plugin/pluginmember.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtWidgets/QAction>

namespace voxie
{
namespace plugin
{

class IPreferencesPlugin
{
public:
    virtual QWidget *preferencesWidget() = 0;
};

class IVisualizerPlugin
{
public:
    virtual QVector<MetaVisualizer*> visualizers() = 0;
};

class IScriptExtensionPlugin
{
public:
    virtual QVector<PluginMember*> scriptExtensions() = 0;
};

class IUICommandPlugin
{
public:
    virtual QVector<QAction*> uiCommands() = 0;
};

class IFilter2DPlugin
{
public:
    virtual QVector<MetaFilter2D*> filters2D() = 0;
};

class IFilter3DPlugin
{
public:
    virtual QVector<MetaFilter3D*> filters3D() = 0;
};

class ILoaderPlugin
{
public:
    virtual QVector<voxie::io::Loader*> loaders() = 0;
};

class IImporterPlugin
{
public:
    virtual QVector<voxie::io::Importer*> importers() = 0;
};

class IVoxelExportPlugin
{
public:
    virtual QVector<voxie::io::VoxelExporter*> voxelExporters() = 0;
};

class ISliceExportPlugin
{
public:
    virtual QVector<voxie::io::SliceExporter*> sliceExporters() = 0;
};

}
}

QT_BEGIN_NAMESPACE

#define V_DECLARE_INTERFACE(iface, name) Q_DECLARE_INTERFACE(voxie::plugin::iface, name)

V_DECLARE_INTERFACE(IPreferencesPlugin, "de.uni_stuttgart.Voxie.Plugin.IPreferencesPlugin")
V_DECLARE_INTERFACE(IVisualizerPlugin, "de.uni_stuttgart.Voxie.Plugin.IVisualizerPlugin")
V_DECLARE_INTERFACE(IScriptExtensionPlugin, "de.uni_stuttgart.Voxie.Plugin.IScriptExtensionPlugin")
V_DECLARE_INTERFACE(IUICommandPlugin, "de.uni_stuttgart.Voxie.Plugin.IUICommandPlugin")
V_DECLARE_INTERFACE(IFilter2DPlugin, "de.uni_stuttgart.Voxie.Plugin.IFilter2DPlugin")
V_DECLARE_INTERFACE(IFilter3DPlugin, "de.uni_stuttgart.Voxie.Plugin.IFilter3DPlugin")
V_DECLARE_INTERFACE(ILoaderPlugin, "de.uni_stuttgart.Voxie.Plugin.ILoaderPlugin")
V_DECLARE_INTERFACE(IImporterPlugin, "de.uni_stuttgart.Voxie.Plugin.IImporterPlugin")
V_DECLARE_INTERFACE(IVoxelExportPlugin, "de.uni_stuttgart.Voxie.Plugin.IVoxelExportPlugin")
V_DECLARE_INTERFACE(ISliceExportPlugin, "de.uni_stuttgart.Voxie.Plugin.ISliceExportPlugin")

QT_END_NAMESPACE

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
