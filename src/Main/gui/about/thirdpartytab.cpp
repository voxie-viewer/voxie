#include "thirdpartytab.hpp"

#include <QtWidgets/QHBoxLayout>

using namespace voxie::gui::about;
using namespace voxie;

ThirdPartyTab::ThirdPartyTab(QWidget *parent) :
    QWidget(parent)
{
    setupElements(setupLayout());
}

QBoxLayout*
ThirdPartyTab::setupLayout()
{
    QBoxLayout* layout = new  QHBoxLayout(this);
    return layout;
}

void
ThirdPartyTab::setupElements(QBoxLayout* layout)
{
     edit_info = new QTextEdit;
     QString info = "Fugue Icons\n";
     info += "\n";
     info += "(C) 2013 Yusuke Kamiyamane. All rights reserved.\n";
     info += "\n";
     info += "These icons are licensed under a Creative Commons\n";
     info += "Attribution 3.0 License.\n";
     info += "<http://creativecommons.org/licenses/by/3.0/>\n";
     info += "\n";
     info += "If you can't or don't want to provide attribution, please\n";
     info += "purchase a royalty-free license.\n";
     info += "<http://p.yusukekamiyamane.com/>\n";
     info += "\n";
     info += "I'm unavailable for custom icon design work. But your\n";
     info += "suggestions are always welcome!\n";
     info += "<mailto:p@yusukekamiyamane.com>\n";
     info += "\n";
     info += "------------------------------------------------------------\n";
     info += "\n";
     info += "All logos and trademarks in some icons are property of their\n";
     info += "respective owners.\n";
     info += "\n";
     info += "------------------------------------------------------------\n";
     info += "\n";
     info += "- geotag\n";
     info += "\n";
     info += "  (C) Geotag Icon Project. All rights reserved.\n";
     info += "  <http://www.geotagicons.com/>\n";
     info += "\n";
     info += "  Geotag icon is licensed under a Creative Commons\n";
     info += "  Attribution-Share Alike 3.0 License or LGPL.\n";
     info += "  <http://creativecommons.org/licenses/by-sa/3.0/>\n";
     info += "  <http://opensource.org/licenses/lgpl-license.php>\n";
     info += "\n";
     info += "- language\n";
     info += "\n";
     info += "  (C) Language Icon Project. All rights reserved.\n";
     info += "  <http://www.languageicon.org/>\n";
     info += "\n";
     info += "  Language icon is licensed under a Creative Commons\n";
     info += "  Attribution-Share Alike 3.0 License.\n";
     info += "  <http://creativecommons.org/licenses/by-sa/3.0/>\n";
     info += "\n";
     info += "- open-share\n";
     info += "\n";
     info += "  (C) Open Share Icon Project. All rights reserved.\n";
     info += "  <http://www.openshareicons.com/>\n";
     info += "\n";
     info += "  Open Share icon is licensed under a Creative Commons\n";
     info += "  Attribution-Share Alike 3.0 License.\n";
     info += "  <http://creativecommons.org/licenses/by-sa/3.0/>\n";
     info += "\n";
     info += "- opml\n";
     info += "\n";
     info += "  (C) OPML Icon Project. All rights reserved.\n";
     info += "  <http://opmlicons.com/>\n";
     info += "\n";
     info += "  OPML icon is licensed under a Creative Commons\n";
     info += "  Attribution-Share Alike 2.5 License.\n";
     info += "  <http://creativecommons.org/licenses/by-sa/2.5/>\n";
     info += "\n";
     info += "- share\n";
     info += "\n";
     info += "  (C) Share Icon Project. All rights reserved.\n";
     info += "  <http://shareicons.com/>\n";
     info += "\n";
     info += "  Share icon is licensed under a GPL or LGPL or BSD or\n";
     info += "  Creative Commons Attribution 2.5 License.\n";
     info += "  <http://opensource.org/licenses/gpl-license.php>\n";
     info += "  <http://opensource.org/licenses/lgpl-license.php>\n";
     info += "  <http://opensource.org/licenses/bsd-license.php>\n";
     info += "  <http://creativecommons.org/licenses/by/2.5/>\n";
     info += "\n";
     info += "- xfn\n";
     info += "\n";
     info += "  (C) Wolfgang Bartelme. All rights reserved.\n";
     info += "  <http://www.bartelme.at/>\n";
     info += "\n";
     info += "  XFN icon is licensed under a Creative Commons\n";
     info += "  Attribution-Share Alike 2.5 License.\n";
     info += "  <http://creativecommons.org/licenses/by-sa/2.5/>\n";

     edit_info->setReadOnly(true);
     edit_info->setText(info);
     edit_info->setAlignment(Qt::AlignAbsolute);

    // place elements
    layout->addWidget(this->edit_info);
}


ThirdPartyTab::~ThirdPartyTab()
{

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
