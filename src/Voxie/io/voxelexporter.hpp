#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/plugin/pluginmember.hpp>

#include <QtCore/QObject>

namespace voxie {
// Forward declarations
namespace data{
class DataSet;
}

namespace io
{

class VOXIECORESHARED_EXPORT VoxelExporter :
        public voxie::plugin::PluginMember
{
    Q_OBJECT
public:
    explicit VoxelExporter(QObject *parent = 0);

    /**
     * @brief Exports a data set from the GUI.
     * Exporter may utilize GUI to receive further export configurations
     * and targets from user.
     * @param dataSet
     */
    Q_INVOKABLE virtual void exportGui(voxie::data::DataSet *dataSet) = 0;

signals:

public slots:

};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
