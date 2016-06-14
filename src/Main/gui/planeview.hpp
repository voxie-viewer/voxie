#pragma once

#include <Voxie/data/slice.hpp>
#include <Voxie/visualization/openglwidget.hpp>
#include <Voxie/visualization/view3d.hpp>

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>

namespace voxie
{
namespace gui
{

class PlaneView : public voxie::visualization::OpenGLWidget
{
    Q_OBJECT
private:
    voxie::data::Slice *slice;

    QPoint mouseLast;

    voxie::visualization::View3D* view3d;

    QOpenGLVertexArrayObject vao;
    QOpenGLShaderProgram program;
    GLuint MVP_ID;
    GLuint vertexPosition_modelspaceID;
    GLuint vertexColorID;
    QOpenGLBuffer vertexbuffer;
    QOpenGLBuffer colorbuffer;

    void draw(GLenum mode, const QVector<GLfloat>& vertices, const QVector<GLfloat>& colors, size_t count, const QMatrix4x4& modelMatrix);

public:
    explicit PlaneView(voxie::data::Slice *slice, QWidget *parent = 0);

protected:

    virtual QString initialize() override;
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
