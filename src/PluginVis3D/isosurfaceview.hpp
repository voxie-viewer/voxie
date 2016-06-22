#pragma once

#include <Voxie/data/dataset.hpp>

#include <Voxie/visualization/openglwidget.hpp>
#include <Voxie/visualization/view3d.hpp>

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>

#include <QtWidgets/QProgressBar>

namespace voxie { namespace data {
    class Surface;
    class SurfaceBuilder;
} }

class IsosurfaceView : public voxie::visualization::OpenGLWidget {
    Q_OBJECT
    voxie::data::DataSet *voxelData;
private:
    bool generating = false;
    QPoint mouseLast;
    QSharedPointer<voxie::data::Surface> surface;

    QProgressBar *progressBar;

    voxie::visualization::View3D* view3d;

    QOpenGLVertexArrayObject vao;
    QOpenGLShaderProgram program;
    GLuint MVP_ID;
    GLuint vertexPosition_modelspaceID;
    GLuint vertexColorID;
    QOpenGLBuffer vertexbuffer;
    QOpenGLBuffer colorbuffer;
    size_t triangleCount;

    void draw(GLenum mode, const QVector<GLfloat>& vertices, const QVector<GLfloat>& colors, size_t count, const QMatrix4x4& modelMatrix);

    void genCube(const QVector3D &pos, int sides, voxie::data::SurfaceBuilder* sb);

    void generateModel();

    void uploadData();

private slots:
    void updateSurface(const QSharedPointer<voxie::data::Surface>& surface);

public:
    explicit IsosurfaceView(voxie::data::DataSet *voxelData, QWidget *parent = 0);

    virtual QString initialize() override;
    virtual void paint() override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void regenerate();

public:
    float threshold;
    bool inverted;
    bool useMarchingCubes;

signals:
    void progressChanged(float x);

    void generationDone(const QSharedPointer<voxie::data::Surface>& surface);
};


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
