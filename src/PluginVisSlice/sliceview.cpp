#include "sliceview.hpp"

#include <PluginVisSlice/slicemetavisualizer.hpp>

#include <QtWidgets/QMessageBox>

SliceView::SliceView(QObject *parent) :
	QGenericPlugin(parent)
{

}



QObject *SliceView::create(const QString & key, const QString & specification)
{
	(void)key;
	(void)specification;
	return nullptr;
}

QVector<QAction*> SliceView::uiCommands()
{
	QVector<QAction*> actions;

	return actions;
}

QVector<voxie::plugin::MetaVisualizer*> SliceView::visualizers()
{
	QVector<voxie::plugin::MetaVisualizer*> list;
	list.append(SliceMetaVisualizer::instance());
	return list;
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(SliceView, SliceView)
#endif // QT_VERSION < 0x050000

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
