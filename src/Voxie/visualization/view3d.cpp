#include "view3d.hpp"

#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>

using namespace voxie::visualization;

View3D::View3D(QObject* parent, float zoomMin, float zoomMax)
: QObject(parent),
    zoomMin(zoomMin),
    zoomMax(zoomMax)
{
    this->pan = (float)(M_PI * 40.0 / 180.0);
    this->tilt = (float)(M_PI * 30.0 / 180.0);
    this->zoom = 2.0f;
}

QMatrix4x4 View3D::viewMatrix(float scaling) {
        QVector3D position = cameraPosition(scaling);

        QMatrix4x4 matView;
        matView.lookAt(position,
                       QVector3D(0.0f, 0.0f, 0.0f),
                       QVector3D(0.0f, 1.0f, 0.0f));
        //qDebug() << "matView" << matView;
        return matView;
}
QMatrix4x4 View3D::projectionMatrix(float scaling, float fWidth, float fHeight) {
        QMatrix4x4 matProj;
        matProj.perspective(80,
                            fWidth / fHeight,
                            0.1f * scaling,
                            10000.0f * scaling);
        //qDebug() << "matProj" << matProj;
        return matProj;
}

QVector3D View3D::cameraPosition(float scaling) {
    QVector3D position(cosf(this->pan) * cosf(this->tilt),
                       sinf(this->tilt),
                       sinf(this->pan) * cosf(this->tilt));
    position *= this->zoom * scaling;
    return position;
}

void View3D::mousePressEvent(const QPoint& mouseLast, QMouseEvent *event) {
    Q_UNUSED(mouseLast);
    Q_UNUSED(event);
}
void View3D::mouseMoveEvent(const QPoint& mouseLast, QMouseEvent *event) {
	int dx = event->x() - mouseLast.x();
	int dy = event->y() - mouseLast.y();

	if (event->buttons() & Qt::LeftButton) {
		this->pan += 0.02f * dx;
		this->tilt += 0.02f * dy;
		if(this->tilt > 0.49f * M_PI)
			this->tilt = (float)(0.49f * M_PI);
		if(this->tilt < -0.49f * M_PI)
			this->tilt = (float)(-0.49f * M_PI);
        emit changed();
	}
}
void View3D::wheelEvent(QWheelEvent *event) {
	this->zoom += 0.1f * event->angleDelta().y() / 120.0f;
    if(this->zoom < zoomMin)
        this->zoom = zoomMin;
    if(this->zoom > zoomMax)
        this->zoom = zoomMax;
    emit changed();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
