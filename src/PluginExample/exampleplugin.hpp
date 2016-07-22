#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtCore/QVector>

#include <QtGui/QGenericPlugin>

class ExamplePlugin :
        public QGenericPlugin,
        public voxie::plugin::IUICommandPlugin,
        public voxie::plugin::IVisualizerPlugin,
        public voxie::plugin::IImporterPlugin,
        public voxie::plugin::ILoaderPlugin,
        public voxie::plugin::IPreferencesPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::IUICommandPlugin)
    Q_INTERFACES(voxie::plugin::IVisualizerPlugin)
    Q_INTERFACES(voxie::plugin::IImporterPlugin)
    Q_INTERFACES(voxie::plugin::ILoaderPlugin)
    Q_INTERFACES(voxie::plugin::IPreferencesPlugin)

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "Example.json")

public:
    ExamplePlugin(QObject *parent = 0);

    virtual QObject *create ( const QString & key, const QString & specification ) override;

    virtual QVector<QAction*> uiCommands() override;

    virtual QVector<voxie::plugin::MetaVisualizer*> visualizers() override;

    virtual QVector<voxie::io::Importer*> importers() override;

    virtual QVector<voxie::io::Loader*> loaders() override;

    virtual QWidget *preferencesWidget() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
