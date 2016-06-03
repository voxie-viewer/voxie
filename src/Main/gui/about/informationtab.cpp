#include "informationtab.hpp"

//#include <QtCore/QUrl>

//#include <QtGui/QDesktopServices>

using namespace voxie::gui::about;
using namespace voxie;

InformationTab::InformationTab(QWidget *parent) :
    QWidget(parent)
{
    setupElements(setupLayout());
}

QGridLayout*
InformationTab::setupLayout()
{
    QGridLayout* layout = new  QGridLayout(this);

    // column spacing
    layout->setColumnMinimumWidth(1, 8);
    layout->setColumnMinimumWidth(2, 20);

    // row spacing
    layout->setRowMinimumHeight(1, 10);
    layout->setRowMinimumHeight(2, 10);
    layout->setRowMinimumHeight(3, 10);
    layout->setRowMinimumHeight(4, 10);
    //layout->setRowMinimumHeight(5, 10);
    return layout;
}

void
InformationTab::setupElements(QGridLayout* layout)
{

    lbl_name    = new QLabel("Name: ");
    lbl_version = new QLabel("Version: ");
    lbl_author  = new QLabel("Author: ");
    lbl_homepage = new QLabel("Homepage: ");
    lbl_documentation = new QLabel("Dokumentation: ");

    lbl_name_val    = new QLabel("Voxie");
    lbl_version_val = new QLabel("Version 0.1 rev " + QString(GIT_VERSION) + " built with QT " + QString(QT_VERSION_STR));
    lbl_author_val  = new QLabel("Andreas Korge, Patrick Karner, Hans Martin Berner, Daniel Topp, Tim Borner, Felix Queißner, David Hägele, Steffen Kieß");
    lbl_homepage_val    = new QLabel;
    lbl_homepage_val->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse);
    lbl_homepage_val->setOpenExternalLinks(true);
    lbl_homepage_val->setText("<a href=\"https://github.com/voxie-viewer/voxie\">https://github.com/voxie-viewer/voxie</a>");
    /*
    lbl_documentation_val    = new QLabel;
    lbl_documentation_val->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse);
    lbl_documentation_val->setOpenExternalLinks(true);
    lbl_documentation_val->setText("<a href=\"http://\">http://</a>");
    */

    layout->addWidget(lbl_name, 0, 0);
    layout->addWidget(lbl_version, 1, 0);
    layout->addWidget(lbl_author, 2, 0);
    layout->addWidget(lbl_homepage, 3, 0);
    //layout->addWidget(lbl_documentation, 4, 0);

    layout->addWidget(lbl_name_val, 0, 1);
    layout->addWidget(lbl_version_val, 1, 1);
    layout->addWidget(lbl_author_val, 2, 1);
    layout->addWidget(lbl_homepage_val, 3, 1);
    //layout->addWidget(lbl_documentation_val, 4, 1);
}


InformationTab::~InformationTab()
{

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
