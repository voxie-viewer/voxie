#pragma once

#include <Voxie/data/dataset.hpp>

#include <Voxie/visualization/view3d.hpp>

#include <QtOpenGL/QGLWidget>

#include <QtWidgets/QProgressBar>

class IsosurfaceView :
        public QGLWidget
{
    Q_OBJECT
    voxie::data::DataSet *voxelData;
private:
    bool generating = false;
    float fWidth, fHeight;
    QPoint mouseLast;
    QVector<GLuint> lists;

    QProgressBar *progressBar;

    voxie::visualization::View3D* view3d;

    void genCube(const QVector3D &pos, int sides);

    void generateModel();
public:
    explicit IsosurfaceView(voxie::data::DataSet *voxelData, QWidget *parent = 0);

    virtual void glInit() override;
    virtual void glDraw() override;

    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;

    void regenerate();

public:
    float threshold;
    bool inverted;
    bool useMarchingCubes;

signals:
    void progressChanged(int x);

public slots:

};


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
