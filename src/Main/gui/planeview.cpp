#include "planeview.hpp"

#include <cmath>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>

#include <QtOpenGL/QGLFormat>

using namespace voxie::gui;
using namespace voxie::data;

PlaneView::PlaneView(Slice *slice, QWidget *parent) :
    OpenGLDrawWidget(parent),
    slice(slice),
    view3d(new voxie::visualization::View3D(this, false, this->slice->getDataset()->diagonalSize(), 0.3f, 1.0f))
{
    this->setMinimumHeight(150);
    QMetaObject::Connection conni = connect(this->slice, &QObject::destroyed, [this]() -> void
    {
        this->slice = nullptr;
    });
    connect(this, &QObject::destroyed, [=]() -> void
    {
        this->disconnect(conni);
    });

    connect(view3d, &voxie::visualization::View3D::changed, this, [this] { this->repaint(); });
}

void PlaneView::mousePressEvent(QMouseEvent *event)
{
    view3d->mousePressEvent(mouseLast, event, size());
    this->mouseLast = event->pos();
}

void PlaneView::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - this->mouseLast.x();
    int dy = event->y() - this->mouseLast.y();

    if((event->buttons() & Qt::RightButton) && (this->slice != nullptr))
    {
        float ax = 0.12f * dx;
        float ay = 0.12f * dy;

        QQuaternion src = this->slice->rotation();

        QMatrix4x4 matView = view3d->viewMatrix();
        matView.setRow(3, QVector4D(0,0,0,1)); // Remove translation

        QQuaternion quatX = QQuaternion::fromAxisAndAngle((QVector4D(0, 1, 0, 0) * matView).toVector3D(), ax);
        QQuaternion quatY = QQuaternion::fromAxisAndAngle((QVector4D(1, 0, 0, 0) * matView).toVector3D(), ay);

        QQuaternion quat = quatX * quatY;

        this->slice->setRotation(quat * src);

        this->repaint();
    } else {
        view3d->mouseMoveEvent(mouseLast, event, size());
    }
    this->mouseLast = event->pos();
}

void PlaneView::wheelEvent(QWheelEvent *event)
{
    view3d->wheelEvent(event, size());
}

void PlaneView::paint() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepthf(1.0f);

    if(this->slice == nullptr)
        return;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    QVector3D extends = this->slice->getDataset()->size();
    QVector3D origin = this->slice->getDataset()->origin();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    PrimitiveBuffer buffer;

    QMatrix4x4 matViewProj = view3d->projectionMatrix(this->width(), this->height()) * view3d->viewMatrix();

    QMatrix4x4 transformDataSet = matViewProj;
    transformDataSet.translate(origin);

    ///////////////////////////////////////////////////////////////////////////////////
    /// Render data set cuboid
    ///////////////////////////////////////////////////////////////////////////////////

    buffer.clear();

    // Front
    buffer.addQuad(QVector3D(0.8f, 0.8f, 0.8f),
                   QVector3D(0,           0,           extends.z()),
                   QVector3D(extends.x(), 0,           extends.z()),
                   QVector3D(extends.x(), extends.y(), extends.z()),
                   QVector3D(0,           extends.y(), extends.z()));

    // Back
    buffer.addQuad(QVector3D(0.4f, 0.4f, 0.4f),
                   QVector3D(0,           0,           0),
                   QVector3D(extends.x(), 0,           0),
                   QVector3D(extends.x(), extends.y(), 0),
                   QVector3D(0,           extends.y(), 0));

    // Left
    buffer.addQuad(QVector3D(0.6f, 0.6f, 0.6f),
                   QVector3D(extends.x(), 0,           0),
                   QVector3D(extends.x(), 0,           extends.z()),
                   QVector3D(extends.x(), extends.y(), extends.z()),
                   QVector3D(extends.x(), extends.y(), 0));

    // Right
    buffer.addQuad(QVector3D(0.5f, 0.5f, 0.5f),
                   QVector3D(0, 0,           0),
                   QVector3D(0, 0,           extends.z()),
                   QVector3D(0, extends.y(), extends.z()),
                   QVector3D(0, extends.y(), 0));

    // Top
    buffer.addQuad(QVector3D(0.7f, 0.7f, 0.7f),
                   QVector3D(0,           extends.y(), 0),
                   QVector3D(0,           extends.y(), extends.z()),
                   QVector3D(extends.x(), extends.y(), extends.z()),
                   QVector3D(extends.x(), extends.y(), 0));

    // Bottom
    buffer.addQuad(QVector3D(0.3f, 0.3f, 0.3f),
                   QVector3D(0,           0,  0),
                   QVector3D(0,           0, extends.z()),
                   QVector3D(extends.x(), 0, extends.z()),
                   QVector3D(extends.x(), 0, 0));

    draw(buffer, transformDataSet);


    ///////////////////////////////////////////////////////////////////////////////////
    /// Render coordinate axis
    ///////////////////////////////////////////////////////////////////////////////////

    buffer.clear();

    buffer.addLine(QVector3D(1.0f, 1.0f, 1.0f),
                   QVector3D(0.0f, 0.0f, 0.0f),
                   QVector3D(2.0f * extends.x(), 0.0f, 0.0f));

    buffer.addLine(QVector3D(1.0f, 1.0f, 1.0f),
                   QVector3D(0.0f, 0.0f, 0.0f),
                   QVector3D(0.0f, 2.0f * extends.y(), 0.0f));

    buffer.addLine(QVector3D(1.0f, 1.0f, 1.0f),
                   QVector3D(0.0f, 0.0f, 0.0f),
                   QVector3D(0.0f, 0.0f, 2.0f * extends.z()));

    draw(buffer, matViewProj);

    ///////////////////////////////////////////////////////////////////////////////////
    /// Render plane
    ///////////////////////////////////////////////////////////////////////////////////

    QMatrix4x4 transformPlane = matViewProj;

    // Adjust origin
    //transformPlane.translate(origin);

    // Slice transformation
    transformPlane.translate(this->slice->origin());
    transformPlane.rotate(this->slice->rotation());

    buffer.clear();

    buffer.addQuad(QVector4D(1.0f, 0.0f, 0.0f, 0.4f),
                   QVector3D(-extends.length(), -extends.length(), 0),
                   QVector3D(-extends.length(), extends.length(),  0),
                   QVector3D( extends.length(), extends.length(),  0),
                   QVector3D( extends.length(), -extends.length(), 0));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    draw(buffer, transformPlane);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    buffer.clear();

    buffer.addLine(QVector3D(0.0f, 1.0f, 0.0f),
                   QVector3D(0.0f, 0.0f, 0.0f),
                   QVector3D(extends.length(), 0.0f, 0.0f));

    buffer.addLine(QVector3D(0.0f, 0.0f, 1.0f),
                   QVector3D(0.0f, 0.0f, 0.0f),
                   QVector3D(0.0f, extends.length(), 0.0f));

    buffer.addLine(QVector3D(1.0f, 0.0f, 0.0f),
                   QVector3D(0.0f, 0.0f, 0.0f),
                   QVector3D(0.0f, 0.0f, extends.length()));

    draw(buffer, transformPlane);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
