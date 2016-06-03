#pragma once

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

namespace voxie
{
namespace gui
{
namespace about
{
/**
 * @brief The InformationTab class provides the "information" tab for the about dialog
 * @author Tim Borner
 */
class InformationTab : public QWidget
{
    Q_OBJECT
public:
    explicit InformationTab(QWidget *parent = 0);
    ~InformationTab();


private:
    QGridLayout *setupLayout();
    void setupElements(QGridLayout* layout);

    QLabel* lbl_name;
    QLabel* lbl_version;
    QLabel* lbl_author;
    QLabel* lbl_homepage;
    QLabel* lbl_documentation;

    QLabel* lbl_name_val;
    QLabel* lbl_version_val;
    QLabel* lbl_author_val;
    QLabel* lbl_homepage_val;
    QLabel* lbl_documentation_val;
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
