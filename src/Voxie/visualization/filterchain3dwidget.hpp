#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/filter/filterchain3d.hpp>

#include <QtWidgets/QAction>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

namespace voxie {
namespace visualization {

/**
 * Widget to display and modify a FilterChain3D
 */
class VOXIECORESHARED_EXPORT FilterChain3DWidget : public QWidget
{
    Q_OBJECT
private:
    QAction* _directFilterApply;
    voxie::filter::FilterChain3D* filterchain;
    QListWidget *list;
    QDialog *addDialog;
    QListWidget *filterToAdd;

public:
    FilterChain3DWidget(QWidget *parent = 0);
    voxie::filter::FilterChain3D* getFilterChain();

public slots:
    void addFilter();
    void removeFilter();
    void moveFilterUp();
    void moveFilterDown();
    void updateList();
    void checkEnabled(QListWidgetItem* item);
    void openSettingsDialog();
    void openFiltermaskEditor();
    void applyFilter(data::DataSet *dataSet);
    void triggerChainChangeSignal();
    void exportFilterChain();
    void importFilterChain();
};

/**
 * Widget to modify Filter3D's masks
 */
class VOXIECORESHARED_EXPORT Filter3DMaskEditor: public QWidget
{
    Q_OBJECT
public:
    Filter3DMaskEditor(QWidget *parent = 0);
    void setMask(voxie::filter::Selection3DMask* mask);
private:
    voxie::filter::Selection3DMask* mask;
    QListWidget* shapeList;
    QComboBox* cmbShape;

    void setupList();
    void addShape();
    void removeShape();
    void editShape();

    void onMaskChange();
};

}
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
