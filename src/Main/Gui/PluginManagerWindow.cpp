/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "PluginManagerWindow.hpp"

#include <Main/Root.hpp>

#include <Voxie/Component/Plugin.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

using namespace vx;
using namespace vx::gui;
using namespace vx::plugin;

PluginManagerWindow::PluginManagerWindow(QWidget* parent) : QDialog(parent) {
  this->resize(500, 450);
  QVBoxLayout* layout = new QVBoxLayout();
  {
    QTreeWidget* tree = new QTreeWidget();
    tree->setColumnCount(3);
    tree->setColumnWidth(0, 120);
    tree->setColumnWidth(1, 100);
    tree->setColumnWidth(2, 100);
    {
      for (const auto& plugin : ::vx::Root::instance()->plugins()) {
        QTreeWidgetItem* plugin_item = new QTreeWidgetItem(tree);
        plugin_item->setText(0, plugin->name());
        auto scriptExtensions = plugin->scriptExtensions();
        if (scriptExtensions.size() > 0) {
          QTreeWidgetItem* scripts_item = new QTreeWidgetItem(plugin_item);
          scripts_item->setText(0, "Script Extensions");
          for (auto extension : scriptExtensions) {
            QTreeWidgetItem* script_child_item =
                new QTreeWidgetItem(scripts_item);
            script_child_item->setText(0, extension->objectName());
          }
        }
        auto uiCommands = plugin->uiCommands();
        if (uiCommands.size() > 0) {
          QTreeWidgetItem* cmd_item = new QTreeWidgetItem(plugin_item);
          cmd_item->setText(0, "UI Commands");
          for (auto cmds : uiCommands) {
            QTreeWidgetItem* cmds_child_item = new QTreeWidgetItem(cmd_item);
            cmds_child_item->setText(0, cmds->objectName());
          }
        }
        auto importers = plugin->listComponentsTyped<vx::io::Importer>();
        if (importers.size() > 0) {
          QTreeWidgetItem* importer_item = new QTreeWidgetItem(plugin_item);
          importer_item->setText(0, "Importers");
          for (auto importer : importers) {
            QTreeWidgetItem* importer_child_item =
                new QTreeWidgetItem(importer_item);
            importer_child_item->setText(0, importer->objectName());
            importer_child_item->setText(1, importer->filter().filterString());
          }
        }
        auto sliceExporters =
            plugin->listComponentsTyped<vx::io::SliceExporter>();
        if (sliceExporters.size() > 0) {
          QTreeWidgetItem* exporter_item = new QTreeWidgetItem(plugin_item);
          exporter_item->setText(0, "Slice Exporter");
          for (auto exporter : sliceExporters) {
            QTreeWidgetItem* exporter_child_item =
                new QTreeWidgetItem(exporter_item);
            exporter_child_item->setText(0, exporter->objectName());
          }
        }
        auto filters2d =
            plugin->listComponentsTyped<vx::plugin::MetaFilter2D>();
        if (filters2d.size() > 0) {
          QTreeWidgetItem* filter_item = new QTreeWidgetItem(plugin_item);
          filter_item->setText(0, "2D Filter");
          for (auto filter : filters2d) {
            QTreeWidgetItem* filter_child_item =
                new QTreeWidgetItem(filter_item);
            filter_child_item->setText(0, filter->objectName());
          }
        }
        auto objectPrototypes = plugin->listComponentsTyped<NodePrototype>();
        if (objectPrototypes.size() > 0) {
          QTreeWidgetItem* filter_item = new QTreeWidgetItem(plugin_item);
          filter_item->setText(0, "Object Prototypes");
          for (auto filter : objectPrototypes) {
            QTreeWidgetItem* filter_child_item =
                new QTreeWidgetItem(filter_item);
            filter_child_item->setText(0, filter->objectName());
          }
        }
      }
    }
    layout->addWidget(tree);
  }
  {
    QHBoxLayout* hbox = new QHBoxLayout();

    hbox->addSpacerItem(
        new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    layout->addLayout(hbox);
  }
  this->setLayout(layout);
}

PluginManagerWindow::~PluginManagerWindow() {}
