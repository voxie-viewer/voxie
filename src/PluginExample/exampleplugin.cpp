#include "exampleplugin.hpp"

#include <PluginExample/examplemetavisualizer.hpp>
#include <PluginExample/rawimporter.hpp>
#include <PluginExample/thespheregenerator.hpp>

#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>

ExamplePlugin::ExamplePlugin(QObject *parent) :
	QGenericPlugin(parent)
{

}



QObject *ExamplePlugin::create(const QString & key, const QString & specification)
{
	(void)key;
	(void)specification;
	return nullptr;
}

QWidget *ExamplePlugin::preferencesWidget()
{
	QLabel *label = new QLabel("Settings Page");
	label->setAlignment(Qt::AlignCenter);
	return label;
}

QVector<QAction*> ExamplePlugin::uiCommands()
{
	QVector<QAction*> actions;

	QAction *messageBoxAction = new QAction("Show Message Box", nullptr);
	connect(messageBoxAction, &QAction::triggered, []() -> void
	{
		QMessageBox(QMessageBox::Information, "Example Plugin", "Plugins can be integrated into UI!", QMessageBox::Ok).exec();
	});
    messageBoxAction->setObjectName("MessageBox testcommand");
	actions.append(messageBoxAction);

	return actions;
}

QVector<voxie::plugin::MetaVisualizer*> ExamplePlugin::visualizers()
{
	QVector<voxie::plugin::MetaVisualizer*> list;
	list.append(ExampleMetaVisualizer::instance());
	return list;
}

QVector<voxie::io::Importer*> ExamplePlugin::importers()
{
	QVector<voxie::io::Importer*> list;
	list.append(new TheSphereGenerator());
	return list;
}

QVector<voxie::io::Loader*> ExamplePlugin::loaders()
{
	QVector<voxie::io::Loader*> list;
	list.append(new RAWImporter());
	return list;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
