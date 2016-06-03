#pragma once

#include <Voxie/data/range.hpp>
#include <Voxie/data/slice.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>

namespace voxie
{
namespace gui
{

class DataSelectionDialog : public QDialog
{
    Q_OBJECT
    voxie::data::Range sliceCount;
    voxie::data::Range dataSetCount;

    QVector<voxie::data::Slice*> sliceSelection;
    QVector<voxie::data::DataSet*> dataSetSelection;

    QTreeWidget *treeView;
    QPushButton *okButton;
public:
    explicit DataSelectionDialog(voxie::data::Range sliceCount, voxie::data::Range dataSetCount, QWidget *parent = 0);


    /**
     * @brief Gets all selected slices.
     * @return
     */
    QVector<voxie::data::Slice*> slices() const
    {
        return this->sliceSelection;
    }

    /**
     * @brief Gets all selected data sets.
     * @return
     */
    QVector<voxie::data::DataSet*> dataSets() const
    {
        return this->dataSetSelection;
    }

signals:

public slots:

private slots:
    void itemSelectionChanged();

};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
