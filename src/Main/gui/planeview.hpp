#pragma once

#include <Voxie/data/slice.hpp>

#include <QtOpenGL/QGLWidget>

namespace voxie
{
namespace gui
{

class PlaneView : public QGLWidget
{
    Q_OBJECT
private:
    voxie::data::Slice *slice;
    float fWidth, fHeight;
    float pan, tilt, zoom;

    QPoint mouseLast;
public:
    explicit PlaneView(voxie::data::Slice *slice, QWidget *parent = 0);

protected:

    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

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
