#include "isosurfacemetavisualizer.hpp"

#include <PluginVis3D/isosurfacevisualizer.hpp>

IsosurfaceMetaVisualizer::IsosurfaceMetaVisualizer(QWidget *parent) :
	MetaVisualizer(parent)
{
}

voxie::visualization::Visualizer *IsosurfaceMetaVisualizer::createVisualizer(
		const QVector<voxie::data::DataSet*> &dataSets,
		const QVector<voxie::data::Slice*> &slices)
{
	(void)slices;
	return new IsosurfaceVisualizer(dataSets.at(0));
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
