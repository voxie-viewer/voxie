#include "licensetab.hpp"

#include <QtWidgets/QHBoxLayout>

using namespace voxie::gui::about;
using namespace voxie;

AboutLicenseTab::AboutLicenseTab(QWidget *parent) :
    QWidget(parent)
{
    setupElements(setupLayout());
}

QBoxLayout*
AboutLicenseTab::setupLayout()
{
    QBoxLayout* layout = new  QHBoxLayout(this);
    return layout;
}

void
AboutLicenseTab::setupElements(QBoxLayout* layout)
{
     edit_license = new QTextEdit;
     QString license = "This Software is provided under the terms of the MIT license.";
             license += "\n\n";
             license += "Copyright (c) 2015 University of Stuttgart IPVS StuPro CT Team, Steffen Kieß\n";
             license += "\n";
             license += "Permission is hereby granted, free of charge, to any person obtaining a copy\n";
             license += "of this software and associated documentation files (the \"Software\"), to deal\n";
             license += "in the Software without restriction, including without limitation the rights\n";
             license += "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n";
             license += "copies of the Software, and to permit persons to whom the Software is\n";
             license += "furnished to do so, subject to the following conditions:\n";
             license += "\n";
             license += "The above copyright notice and this permission notice shall be included in\n";
             license += "all copies or substantial portions of the Software.\n";
             license += "\n";
             license += "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n";
             license += "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n";
             license += "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n";
             license += "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n";
             license += "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n";
             license += "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n";
             license += "THE SOFTWARE.\n";

     edit_license->setReadOnly(true);
     edit_license->setText(license);
     edit_license->setAlignment(Qt::AlignAbsolute);

    // place elements
    layout->addWidget(this->edit_license);
}


AboutLicenseTab::~AboutLicenseTab()
{

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
