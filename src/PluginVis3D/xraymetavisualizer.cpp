#include "xraymetavisualizer.hpp"

#include <PluginVis3D/xrayvisualizer.hpp>

using namespace voxie::plugin;

XRayMetaVisualizer::XRayMetaVisualizer(QObject *parent) :
    MetaVisualizer(parent)
{
}


voxie::visualization::Visualizer *XRayMetaVisualizer::createVisualizer(
        const QVector<voxie::data::DataSet*> &dataSets,
        const QVector<voxie::data::Slice*> &slices)
{
    (void)slices;
    return new XRayVisualizer(dataSets.at(0));
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
