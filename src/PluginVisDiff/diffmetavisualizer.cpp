#include "diffmetavisualizer.hpp"

#include <PluginVisDiff/diffvisualizer.hpp>

using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::visualization;

DiffMetaVisualizer::DiffMetaVisualizer(QObject *parent) :
    MetaVisualizer(parent)
{

}

DiffMetaVisualizer::~DiffMetaVisualizer()
{

}

Visualizer *DiffMetaVisualizer::createVisualizer(const QVector<DataSet*> &dataSets, const QVector<Slice*> &slices)
{
    (void)dataSets;
    return new DiffVisualizer(slices);
}




// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
