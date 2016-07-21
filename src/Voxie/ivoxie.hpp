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
    class DataObject;
}
namespace io
{
    class Operation;
    class Loader;
}
namespace plugin
{
    class VoxiePlugin;
}

class VOXIECORESHARED_EXPORT ActiveVisualizerProvider : public QObject {
    Q_OBJECT

public:
    virtual voxie::visualization::Visualizer* activeVisualizer() const = 0;
    ~ActiveVisualizerProvider();

signals:
    void activeVisualizerChanged(voxie::visualization::Visualizer* current);
};

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

    virtual void registerDataObject(voxie::data::DataObject *obj) = 0;


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

    virtual bool disableOpenGL() const = 0;
    virtual bool disableOpenCL() const = 0;

    virtual ActiveVisualizerProvider* activeVisualizerProvider() const = 0;

    virtual QWidget* mainWindow() const = 0;

    virtual void addProgressBar(voxie::io::Operation* operation) = 0;

    virtual QObject* createLoaderAdaptor(voxie::io::Loader* loader) = 0;
};

void VOXIECORESHARED_EXPORT setVoxieRoot(IVoxie *voxie);

VOXIECORESHARED_EXPORT IVoxie &voxieRoot();

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
