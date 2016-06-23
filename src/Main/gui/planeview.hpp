#pragma once

#include <Voxie/data/slice.hpp>
#include <Voxie/visualization/openglwidget.hpp>
#include <Voxie/visualization/view3d.hpp>

namespace voxie
{
namespace gui
{

class PlaneView : public voxie::visualization::OpenGLDrawWidget
{
    Q_OBJECT
private:
    voxie::data::Slice *slice;

    QPoint mouseLast;

    voxie::visualization::View3D* view3d;

public:
    explicit PlaneView(voxie::data::Slice *slice, QWidget *parent = 0);

protected:

    virtual void paint() override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

signals:

public slots:

};


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
