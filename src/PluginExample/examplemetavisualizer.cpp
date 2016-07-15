#include "examplemetavisualizer.hpp"

#include <PluginExample/examplevisualizer.hpp>

using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::visualization;

ExampleMetaVisualizer::ExampleMetaVisualizer(QObject *parent) :
	MetaVisualizer(parent)
{

}

ExampleMetaVisualizer::~ExampleMetaVisualizer()
{

}

ExampleMetaVisualizer* ExampleMetaVisualizer::instance() {
    static auto inst = new ExampleMetaVisualizer();
    return inst;
}

Visualizer *ExampleMetaVisualizer::createVisualizer(const QVector<DataSet*> &dataSets, const QVector<Slice*> &slices)
{
	(void)dataSets;
	(void)slices;
	return new ExampleVisualizer();
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
