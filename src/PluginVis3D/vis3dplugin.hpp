#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtGui/QGenericPlugin>

class Vis3DPlugin :
        public QGenericPlugin,
        public voxie::plugin::IVisualizerPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::IVisualizerPlugin)

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "Vis3D.json")

public:
    Vis3DPlugin(QObject *parent = 0);

    virtual QObject* create(const QString& name, const QString &spec) override;

    virtual QVector<voxie::plugin::MetaVisualizer*> visualizers() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
