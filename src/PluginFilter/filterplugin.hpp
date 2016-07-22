#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtCore/QVector>

#include <QtGui/QGenericPlugin>

class FilterPlugin :
        public QGenericPlugin,
        public voxie::plugin::IFilter2DPlugin,
        public voxie::plugin::IFilter3DPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::IFilter2DPlugin)
    Q_INTERFACES(voxie::plugin::IFilter3DPlugin)

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "Filter.json")

public:
    FilterPlugin(QObject *parent = 0);

    virtual QObject *create ( const QString & key, const QString & specification ) override;

    virtual QVector<voxie::plugin::MetaFilter2D*> filters2D() override;

    virtual QVector<voxie::plugin::MetaFilter3D*> filters3D() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
