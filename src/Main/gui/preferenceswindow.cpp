#include "preferenceswindow.hpp"

#include <Main/root.hpp>

#include <Main/gui/preferences/openclpreferences.hpp>
#include <Main/gui/preferences/scriptpreferences.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

using namespace voxie;
using namespace voxie::gui;
using namespace voxie::plugin;
using namespace voxie::gui::preferences;
using namespace voxie::visualization;

PreferencesWindow::PreferencesWindow(QWidget *parent) :
	QDialog(parent)
{
	this->resize(500, 450);
	QVBoxLayout *layout = new QVBoxLayout();
	{
		QTabWidget *tabs  = new QTabWidget(this);
		{
			tabs->addTab(new ScriptPreferences(), "Scripting");
			tabs->addTab(new OpenclPreferences(), "OpenCL");


			for(VoxiePlugin *plugin : ::voxie::Root::instance()->plugins())
			{
				QWidget *page = plugin->preferencesWidget();
				if(page == nullptr)
				{
					continue;
				}
				tabs->addTab(page, plugin->name());
			}
		}
		layout->addWidget(tabs);
	}
	{
		QHBoxLayout *hbox = new QHBoxLayout();

		hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

		{
			QPushButton *cancelButton = new QPushButton("Cancel");
			connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
			hbox->addWidget(cancelButton);
		}
		{
			QPushButton *okButton = new QPushButton("Ok");
			connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
			hbox->addWidget(okButton);
		}


		layout->addLayout(hbox);
	}
	this->setLayout(layout);
}

PreferencesWindow::~PreferencesWindow()
{

}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
