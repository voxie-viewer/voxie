#pragma once

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

namespace voxie
{
namespace gui
{
namespace about
{
/**
 * @brief The AboutLicenseTab class provides the "license" tab for the about dialog
 * @author Tim Borner
 */
class AboutLicenseTab : public QWidget
{
    Q_OBJECT
public:
    explicit AboutLicenseTab(QWidget *parent = 0);
    ~AboutLicenseTab();

private:
    QBoxLayout *setupLayout();
    void setupElements(QBoxLayout* layout);

    QTextEdit* edit_license;
signals:

private slots:
};


}
}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
