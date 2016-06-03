#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/data/slice.hpp>

#include <Voxie/plugin/pluginmember.hpp>

#include <QtCore/QObject>

namespace voxie
{
namespace io
{

class VOXIECORESHARED_EXPORT SliceExporter :
        public voxie::plugin::PluginMember
{
    Q_OBJECT
public:
    explicit SliceExporter(QObject *parent = 0);

    /**
     * @brief Exports a slice from the GUI.
     * Exporter may utilize GUI to receive further export configurations
     * and targets from user.
     * @param dataSet
     */
    Q_INVOKABLE virtual void exportGui(voxie::data::Slice *slice) = 0;

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
