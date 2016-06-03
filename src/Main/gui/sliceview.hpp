#pragma once

#include <Main/gui/planeview.hpp>

#include <Voxie/data/slice.hpp>

#include <Voxie/visualization/qveclineedit.hpp>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

namespace voxie
{
namespace gui
{

class SliceView : public QWidget
{
    Q_OBJECT
    voxie::data::Slice *slice;
    voxie::visualization::QVecLineEdit *rotationEdit;
    voxie::visualization::QVecLineEdit *positionEdit;
    PlaneView *planeView;
public:
    explicit SliceView(voxie::data::Slice *slice, QWidget *parent = 0);
    ~SliceView();

signals:

public slots:

private slots:
    void positionEdited();

    void rotationEdited();

    void sliceChanged();
};


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
