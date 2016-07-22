#include "vissliceplugin.hpp"

#include <PluginVisSlice/slicemetavisualizer.hpp>

#include <QtWidgets/QMessageBox>

VisSlicePlugin::VisSlicePlugin(QObject *parent) :
	QGenericPlugin(parent)
{

}



QObject *VisSlicePlugin::create(const QString & key, const QString & specification)
{
	(void)key;
	(void)specification;
	return nullptr;
}

QVector<QAction*> VisSlicePlugin::uiCommands()
{
	QVector<QAction*> actions;

	return actions;
}

QVector<voxie::plugin::MetaVisualizer*> VisSlicePlugin::visualizers()
{
	QVector<voxie::plugin::MetaVisualizer*> list;
	list.append(SliceMetaVisualizer::instance());
	return list;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
