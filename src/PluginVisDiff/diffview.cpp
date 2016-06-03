#include "diffview.hpp"

#include <PluginVisDiff/diffmetavisualizer.hpp>

#include <QtWidgets/QMessageBox>

DiffView::DiffView(QObject *parent) :
    QGenericPlugin(parent)
{

}



QObject *DiffView::create(const QString & key, const QString & specification)
{
    (void)key;
    (void)specification;
    return nullptr;
}

QVector<QAction*> DiffView::uiCommands()
{
    QVector<QAction*> actions;

    return actions;
}

QVector<voxie::plugin::MetaVisualizer*> DiffView::visualizers()
{
    QVector<voxie::plugin::MetaVisualizer*> list;
    list.append(new DiffMetaVisualizer());
    return list;
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(DiffView, DiffView)
#endif // QT_VERSION < 0x050000

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
