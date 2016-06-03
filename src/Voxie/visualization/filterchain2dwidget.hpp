#pragma once

#include <Voxie/data/floatimage.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <Voxie/filter/filterchain2d.hpp>

#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

namespace voxie {
namespace visualization {

/**
 * Widget to display and modify a FilterChain2D
 */
class VOXIECORESHARED_EXPORT FilterChain2DWidget : public QWidget
{
    Q_OBJECT

private:
    voxie::filter::FilterChain2D* filterchain;
    QThread* filterchainThread;
    QListWidget *list;
    QDialog *addDialog;
    QListWidget *filterToAdd;

public:
    FilterChain2DWidget(QWidget *parent = 0);
    voxie::filter::FilterChain2D* getFilterChain();

public slots:
    void addFilter();
    void removeFilter();
    void moveFilterUp();
    void moveFilterDown();
    void updateList();
    void checkEnabled(QListWidgetItem* item);
    void openSettingsDialog();
    void openFiltermaskEditor();
    void applyFilter(voxie::data::SliceImage slice);
    void exportFilterChain();
    void importFilterChain();
signals:
    void requestFilterMaskEditor(voxie::filter::Selection2DMask* mask);
};
}
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
