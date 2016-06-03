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
 * @brief The ThirdPartyTab class provides the tab with information about used third party libs in the about dialog
 * @author Tim Borner
 */
class ThirdPartyTab : public QWidget
{
    Q_OBJECT
public:
    explicit ThirdPartyTab(QWidget *parent = 0);
    ~ThirdPartyTab();

private:
    QBoxLayout *setupLayout();
    void setupElements(QBoxLayout* layout);

    QTextEdit *edit_info;
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
