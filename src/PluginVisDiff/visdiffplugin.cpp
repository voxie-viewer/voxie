#include "visdiffplugin.hpp"

#include <PluginVisDiff/diffmetavisualizer.hpp>

#include <QtWidgets/QMessageBox>

VisDiffPlugin::VisDiffPlugin(QObject *parent) :
    QGenericPlugin(parent)
{

}



QObject *VisDiffPlugin::create(const QString & key, const QString & specification)
{
    (void)key;
    (void)specification;
    return nullptr;
}

QVector<QAction*> VisDiffPlugin::uiCommands()
{
    QVector<QAction*> actions;

    return actions;
}

QVector<voxie::plugin::MetaVisualizer*> VisDiffPlugin::visualizers()
{
    QVector<voxie::plugin::MetaVisualizer*> list;
    list.append(DiffMetaVisualizer::instance());
    return list;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
