#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtWidgets/QWidget>

namespace voxie
{
namespace visualization
{
    class Visualizer;
}
namespace data
{
    class DataSet;
    class Slice;
}
namespace plugin
{
    class VoxiePlugin;
}

class IVoxie
{
public:
    virtual void registerVisualizer(visualization::Visualizer *visualizer) = 0;

    /**
     * @brief Registers a side panel section.
     * @param section The section to be registered.
     * @param closeable If true, the section can be closed by the user.
     *
     * Registers and shows a section. Voxie takes ownership of the section.
     */
    virtual void registerSection(QWidget *section, bool closeable = false) = 0;

    virtual void registerDataSet(voxie::data::DataSet *dataSet) = 0;

    virtual void registerSlice(voxie::data::Slice *slice) = 0;


    /**
     * @brief Gets a vector with all open data sets.
     * @return
     */
    virtual QVector<voxie::data::DataSet*> dataSets() const = 0;

    /**
     * @brief Gets a vector with all loaded plugins.
     * @return
     */
    virtual QVector<voxie::plugin::VoxiePlugin*> plugins() const = 0;
};

void VOXIECORESHARED_EXPORT setVoxieRoot(IVoxie *voxie);

VOXIECORESHARED_EXPORT IVoxie &voxieRoot();

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
