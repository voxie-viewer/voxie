#include "pluginmanagerwindow.hpp"

#include <Main/root.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

using namespace voxie;
using namespace voxie::gui;
using namespace voxie::plugin;
using namespace voxie::visualization;

PluginManagerWindow::PluginManagerWindow(QWidget *parent) :
    QDialog(parent)
{
    this->resize(500, 450);
    QVBoxLayout *layout = new QVBoxLayout();
    {
        QTreeWidget *tree = new QTreeWidget();
        tree->setColumnCount(3);
        tree->setColumnWidth(0,120);
        tree->setColumnWidth(1,100);
        tree->setColumnWidth(2,100);
        {
            for(VoxiePlugin *plugin : ::voxie::Root::instance()->plugins())
            {
                QTreeWidgetItem *plugin_item = new QTreeWidgetItem(tree);
                plugin_item->setText(0, plugin->name());
                auto importers = plugin->importers();
                if(importers.size() > 0)
                {
                    QTreeWidgetItem *import_item = new QTreeWidgetItem(plugin_item);
                    import_item->setText(0, "Importers");
                    for(auto importer : importers)
                    {
                        QTreeWidgetItem *import_child_item = new QTreeWidgetItem(import_item);
                        import_child_item->setText(0,importer->name());
                    }
                }
                auto visualizers = plugin->visualizers();
                if(visualizers.size() > 0)
                {
                    QTreeWidgetItem *import_item = new QTreeWidgetItem(plugin_item);
                    import_item->setText(0, "Visualizer");
                    QString types[4] = {"vtMiscellaneous","vt2D","vt3D","vtAnalytic"};
                    for(auto visualizer : visualizers)
                    {
                        QTreeWidgetItem *vis_child_item = new QTreeWidgetItem(import_item);
                        vis_child_item->setText(0,visualizer->name());

                        QString info = "Type: ";
                        info += types[(visualizer->type() < 3 && visualizer->type() >= 0)? visualizer->type() : 0];
                        if(visualizer->requiredDataSetCount().max > 0)
                            info += " | DataSets: " + QString::number(visualizer->requiredDataSetCount().min) + " - " + QString::number(visualizer->requiredDataSetCount().max);
                        if(visualizer->requiredSliceCount().max > 0)
                        info += " | Slices: " + QString::number(visualizer->requiredSliceCount().min) + " - " + QString::number(visualizer->requiredSliceCount().max);
                        vis_child_item->setText(1, info);
                    }
                }
                auto scriptExtensions = plugin->scriptExtensions();
                if(scriptExtensions.size() > 0)
                {
                    QTreeWidgetItem *scripts_item = new QTreeWidgetItem(plugin_item);
                    scripts_item->setText(0, "Script Extensions");
                    for(auto extension : scriptExtensions)
                    {
                        QTreeWidgetItem *script_child_item = new QTreeWidgetItem(scripts_item);
                        script_child_item->setText(0, extension->objectName());
                    }
                }
                auto uiCommands = plugin->uiCommands();
                if(uiCommands.size() > 0)
                {
                    QTreeWidgetItem *cmd_item = new QTreeWidgetItem(plugin_item);
                    cmd_item->setText(0, "UI Commands");
                    for(auto cmds : uiCommands)
                    {
                        QTreeWidgetItem *cmds_child_item = new QTreeWidgetItem(cmd_item);
                        cmds_child_item->setText(0, cmds->objectName());
                    }
                }
                auto loaders = plugin->loaders();
                if(loaders.size() > 0)
                {
                    QTreeWidgetItem *loader_item = new QTreeWidgetItem(plugin_item);
                    loader_item->setText(0, "Loaders");
                    for(auto loader : loaders)
                    {
                        QTreeWidgetItem *loader_child_item = new QTreeWidgetItem(loader_item);
                        loader_child_item->setText(0, loader->objectName());
                        loader_child_item->setText(1, loader->filter().filterString());
                    }
                }
                auto voxelExporters = plugin->voxelExporters();
                if(voxelExporters.size() > 0)
                {
                    QTreeWidgetItem *exporter_item = new QTreeWidgetItem(plugin_item);
                    exporter_item->setText(0, "Voxel Exporter");
                    for(auto exporter : voxelExporters)
                    {
                        QTreeWidgetItem *exporter_child_item = new QTreeWidgetItem(exporter_item);
                        exporter_child_item->setText(0, exporter->objectName());
                    }
                }
                auto sliceExporters = plugin->sliceExporters();
                if(sliceExporters.size() > 0)
                {
                    QTreeWidgetItem *exporter_item = new QTreeWidgetItem(plugin_item);
                    exporter_item->setText(0, "Slice Exporter");
                    for(auto exporter : sliceExporters)
                    {
                        QTreeWidgetItem *exporter_child_item = new QTreeWidgetItem(exporter_item);
                        exporter_child_item->setText(0, exporter->objectName());
                    }
                }
                auto filters2d = plugin->filters2D();
                if(filters2d.size() > 0)
                {
                    QTreeWidgetItem *filter_item = new QTreeWidgetItem(plugin_item);
                    filter_item->setText(0, "2D Filter");
                    for(auto filter : filters2d)
                    {
                        QTreeWidgetItem *filter_child_item = new QTreeWidgetItem(filter_item);
                        filter_child_item->setText(0, filter->objectName());
                    }
                }
                auto filters3D = plugin->filters3D();
                if(filters3D.size() > 0)
                {
                    QTreeWidgetItem *filter_item = new QTreeWidgetItem(plugin_item);
                    filter_item->setText(0, "3D Filter");
                    for(auto filter : filters3D)
                    {
                        QTreeWidgetItem *filter_child_item = new QTreeWidgetItem(filter_item);
                        filter_child_item->setText(0, filter->objectName());
                    }
                }

            }
        }
        layout->addWidget(tree);
    }
    {
        QHBoxLayout *hbox = new QHBoxLayout();

        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        layout->addLayout(hbox);
    }
    this->setLayout(layout);
}

PluginManagerWindow::~PluginManagerWindow()
{

}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
