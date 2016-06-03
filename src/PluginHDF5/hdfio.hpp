#pragma once

#include <Voxie/plugin/interfaces.hpp>

#include <QtGui/QGenericPlugin>

class HDFIO : public QGenericPlugin,
        public voxie::plugin::ILoaderPlugin,
        public voxie::plugin::IVoxelExportPlugin,
        public voxie::plugin::ISliceExportPlugin
{
    Q_OBJECT
    Q_INTERFACES(voxie::plugin::ILoaderPlugin)
    Q_INTERFACES(voxie::plugin::IVoxelExportPlugin)
    Q_INTERFACES(voxie::plugin::ISliceExportPlugin)
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "HDF-IO.json")
#endif // QT_VERSION >= 0x050000

public:
    HDFIO(QObject *parent = 0);

    virtual QObject *	create ( const QString & key, const QString & specification ) override;

    virtual QVector<voxie::io::Loader*> loaders() override;

    virtual QVector<voxie::io::VoxelExporter*> voxelExporters() override;

    virtual QVector<voxie::io::SliceExporter*> sliceExporters() override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
