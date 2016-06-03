#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtCore/QVector>

#include <QtGui/QGenericPlugin>

class VoxieFilters :
        public QGenericPlugin,
        public voxie::plugin::IFilter2DPlugin,
        public voxie::plugin::IFilter3DPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::IFilter2DPlugin)
    Q_INTERFACES(voxie::plugin::IFilter3DPlugin)

#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "VoxieFilters.json")
#endif // QT_VERSION >= 0x050000

public:
    VoxieFilters(QObject *parent = 0);

    virtual QObject *create ( const QString & key, const QString & specification ) override;

    virtual QVector<voxie::plugin::MetaFilter2D*> filters2D() override;

    virtual QVector<voxie::plugin::MetaFilter3D*> filters3D() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
