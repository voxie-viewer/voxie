#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtGui/QGenericPlugin>

class Voxie3D :
        public QGenericPlugin,
        public voxie::plugin::IVisualizerPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::IVisualizerPlugin)
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "Voxie3D.json")
#endif // QT_VERSION >= 0x050000

public:
    Voxie3D(QObject *parent = 0);

    virtual QObject* create(const QString& name, const QString &spec) override;

    virtual QVector<voxie::plugin::MetaVisualizer*> visualizers() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
