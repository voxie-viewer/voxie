#include "slicemetavisualizer.hpp"

#include <PluginVisSlice/slicevisualizer.hpp>

using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::visualization;

SliceMetaVisualizer::SliceMetaVisualizer(QObject *parent) :
	MetaVisualizer(parent)
{

}

SliceMetaVisualizer::~SliceMetaVisualizer()
{

}

Visualizer *SliceMetaVisualizer::createVisualizer(const QVector<DataSet*> &dataSets, const QVector<Slice*> &slices)
{
	(void)dataSets;
	return new SliceVisualizer(slices);
}




// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
