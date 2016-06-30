#pragma once

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/plane.hpp>

#include <Voxie/visualization/openglwidget.hpp>
#include <Voxie/visualization/view3d.hpp>

#include <QtCore/QTimer>

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>

#include <QtWidgets/QProgressBar>

namespace voxie { namespace data {
    class Surface;
    class SurfaceBuilder;
} }

class IsosurfaceView : public voxie::visualization::OpenGLDrawWidget {
    Q_OBJECT
    voxie::data::DataSet *voxelData;

private:
    bool generating = false;
    bool generationRequested = false;
    QPoint mouseLast;
    QSharedPointer<voxie::data::Surface> surface;

    QProgressBar *progressBar;

    voxie::visualization::View3D* view3d;

    //QOpenGLVertexArrayObject vao;
    QOpenGLShaderProgram program;
    GLuint MVP_ID;
    GLuint vertexPosition_modelspaceID;
    GLuint vertexColorID;
    QOpenGLBuffer surfaceVertexBuffer;
    QOpenGLBuffer surfaceColorBuffer;
    size_t vertexCount;

    bool hasHighlightedPlane = false;
    voxie::data::Plane highlightedPlane;
    QTimer highlightTimer;

    void genCube(const QVector3D &pos, int sides, voxie::data::SurfaceBuilder* sb);

    void generateModel();

    void uploadData();

private slots:
    void updateSurface(const QSharedPointer<voxie::data::Surface>& surface);

    void addSlice(voxie::data::Slice* slice, bool changedNow);
    void updateSlice(const voxie::data::Plane& newPlane);
    void highlightTimeout();

public:
    explicit IsosurfaceView(voxie::data::DataSet *voxelData, QWidget *parent = 0);
    ~IsosurfaceView();

    virtual QString initialize() override;
    virtual void paint() override;
    void paint(const QMatrix4x4& matViewProjection);

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void regenerate();

    // Like QVector3D::normalized(), but with a smaller minimum size
    static QVector3D normalized(const QVector3D& value);

    const QSharedPointer<voxie::data::Surface>& getSurface() const { return surface; }

    enum Culling {
        NO_CULLING, SHOW_FRONT, SHOW_BACK
    };

    voxie::visualization::View3D* getView3D() const { return view3d; }

public:
    float threshold;
    bool inverted;
    bool useMarchingCubes;

    Culling culling;

signals:
    void progressChanged(float x);

    void generationDone(const QSharedPointer<voxie::data::Surface>& surface);
};


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
