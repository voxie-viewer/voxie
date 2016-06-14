#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtGui/QOpenGLFunctions>

#include <QtOpenGL/QGLWidget>

namespace voxie
{
namespace visualization
{

class VOXIECORESHARED_EXPORT OpenGLWidget : public QGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    float fWidth, fHeight;

    bool initialized = false;
    QString initError;

public:
    explicit OpenGLWidget(QWidget *parent = 0);

    float width() const { return fWidth; }
    float height() const { return fHeight; }

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void paintEvent(QPaintEvent *event) override;

    virtual QString initialize()  = 0;
    virtual void paint() = 0;

    bool checkOpenGLStatus();
};


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
