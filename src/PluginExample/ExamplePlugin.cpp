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

#include "ExamplePlugin.hpp"

#include <PluginExample/RAWImporter.hpp>
#include <PluginExample/RandomChartVisualizer.hpp>
#include <PluginExample/TheSphereGenerator.hpp>

#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>

ExamplePlugin::ExamplePlugin(QObject* parent) : QGenericPlugin(parent) {}

QObject* ExamplePlugin::create(const QString& key,
                               const QString& specification) {
  (void)key;
  (void)specification;
  return nullptr;
}

QWidget* ExamplePlugin::preferencesWidget() {
  QLabel* label = new QLabel("Settings Page");
  label->setAlignment(Qt::AlignCenter);
  return label;
}

QList<QAction*> ExamplePlugin::uiCommands() {
  QList<QAction*> actions;

  QAction* messageBoxAction = new QAction("Show Message Box", nullptr);
  connect(messageBoxAction, &QAction::triggered, []() -> void {
    QMessageBox(QMessageBox::Information, "Example Plugin",
                "Plugins can be integrated into UI!", QMessageBox::Ok)
        .exec();
  });
  messageBoxAction->setObjectName("MessageBox testcommand");
  actions.append(messageBoxAction);

  QAction* generateSphereAction = new QAction("Generate Sphere", nullptr);
  connect(generateSphereAction, &QAction::triggered, []() -> void {
    auto filter = dynamic_cast<FilterNode*>(
        TheSphereGenerator::getPrototypeSingleton()
            ->create(QMap<QString, QVariant>(), QList<Node*>(),
                     QMap<QString, QDBusVariant>())
            .data());
    if (filter) filter->run();
  });
  actions.append(generateSphereAction);

  return actions;
}

QList<QSharedPointer<vx::NodePrototype>> ExamplePlugin::objectPrototypes() {
  QList<QSharedPointer<vx::NodePrototype>> list;
  list.append(RandomChartVisualizer::getPrototypeSingleton());
  list.append(TheSphereGenerator::getPrototypeSingleton());
  return list;
}

QList<QSharedPointer<vx::io::Importer>> ExamplePlugin::importers() {
  QList<QSharedPointer<vx::io::Importer>> list;
  list.append(makeSharedQObject<RAWImporter>());
  return list;
}
