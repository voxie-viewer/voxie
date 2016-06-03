#include "aboutdialogwindow.hpp"

#include <Main/root.hpp>

#include <Main/gui/about/informationtab.hpp>
#include <Main/gui/about/licensetab.hpp>
#include <Main/gui/about/thirdpartytab.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

using namespace voxie;
using namespace voxie::gui;
using namespace voxie::visualization;
using namespace voxie::gui::about;


AboutDialogWindow::AboutDialogWindow(QWidget *parent) :
    QDialog(parent)
{
    this->resize(500, 450);
    QVBoxLayout *layout = new QVBoxLayout();
    {
        QTabWidget *tabs  = new QTabWidget(this);
        {
            tabs->addTab(new InformationTab(), "Information");
            tabs->addTab(new AboutLicenseTab(), "License");
            tabs->addTab(new ThirdPartyTab(), "License Fugue Icons");
        }
        layout->addWidget(tabs);
    }
    this->setLayout(layout);
}

AboutDialogWindow::~AboutDialogWindow()
{

}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
