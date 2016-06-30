#include "view3d.hpp"

#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>

using namespace voxie::visualization;

View3D::View3D(QObject* parent, bool allowMove, float viewSizeStandard, float zoomMin, float zoomMax)
: QObject(parent),
    zoomMin(zoomMin),
    zoomMax(zoomMax),
    allowMove(allowMove),
    fieldOfView(40 / 180.0 * M_PI),
    viewSizeStandard(viewSizeStandard),
    zoom(1),
    centerPoint(0, 0, 0),
    rotation(1, 0, 0, 0)
{
    // Set viewSizeStandard to 0.25m
    float factor = 0.25 / this->viewSizeStandard;
    this->viewSizeStandard *= factor;
    this->zoom *= factor;
    this->zoomMin *= factor;
    this->zoomMax *= factor;
}

QMatrix4x4 View3D::viewMatrix() {
        QMatrix4x4 matView;
        matView.translate(0, 0, -distanceCameraCenter());
        matView.scale(zoom);
        matView.rotate(QQuaternion(rotation.scalar(), -rotation.vector()));
        matView.translate(-centerPoint);
        //qDebug() << "matView" << matView;
        return matView;
}
QMatrix4x4 View3D::projectionMatrix(float fWidth, float fHeight) {
        QMatrix4x4 matProj;
        matProj.perspective(fieldOfView / M_PI * 180,
                            fWidth / fHeight,
                            distanceCameraCenter() * 0.1f, // near plane is 1/10 of camera <-> center distance
                            distanceCameraCenter() * 1000); // far plane is 1000 times camera <-> center distance
        //qDebug() << "matProj" << matProj;
        return matProj;
}

// Give a mouse position / window size, return a vector with x, y in [-1;1]
static QVector3D toVector(const QPoint& pos, const QSize& size) {
    return QVector3D(2.0 * (pos.x() + 0.5) / size.width() - 1.0,
                     1.0 - 2.0 * (pos.y() + 0.5) / size.height(),
                     0);
}

// Information about arcball:
// https://en.wikibooks.org/w/index.php?title=OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball&oldid=2356903
static QVector3D getArcballVector(const QPoint& pos, const QSize& size) {
    QVector3D p = toVector(pos, size);
    if (p.lengthSquared() < 1)
        p.setZ(std::sqrt(1 - p.lengthSquared())); // Pythagore
    else
        p.normalize(); // nearest point
    return p;
}

void View3D::mousePressEvent(const QPoint& mouseLast, QMouseEvent *event, const QSize& windowSize) {
    Q_UNUSED(mouseLast);
    Q_UNUSED(event);
    Q_UNUSED(windowSize);
}
void View3D::mouseMoveEvent(const QPoint& mouseLast, QMouseEvent *event, const QSize& windowSize) {
	int dx = event->x() - mouseLast.x();
	int dy = event->y() - mouseLast.y();

    if (dx == 0 && dy == 0)
        return;

	if (event->buttons() & Qt::LeftButton) {
        /*
        rotation = rotation * QQuaternion::fromAxisAndAngle(0, 1, 0, -dx);
        rotation = rotation * QQuaternion::fromAxisAndAngle(1, 0, 0, -dy);
        rotation.normalize();
        */
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (allowMove) {
                QVector3D move(dx, -dy, 0);
                move *= pixelSize(windowSize);
                centerPoint -= rotation.rotatedVector(move);
            }
        } else {
            QVector3D va = getArcballVector(mouseLast, windowSize);
            QVector3D vb = getArcballVector(event->pos(), windowSize);
            float angle = std::acos(std::min(1.0f, QVector3D::dotProduct(va, vb)));
            QVector3D axis = QVector3D::crossProduct(va, vb);
            rotation = rotation * QQuaternion::fromAxisAndAngle(axis, -angle / M_PI * 180);
            rotation.normalize();
        }
        emit changed();
	}
}
void View3D::wheelEvent(QWheelEvent *event, const QSize& windowSize) {
    float mult = 2;
    if (event->modifiers().testFlag(Qt::ShiftModifier))
        mult = 0.2;
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (allowMove) {
            //QVector3D direction(0, 0, 1);
            // Choose direction so that object under the mouse stays the same
            QVector3D direction = -(projectionMatrix(windowSize.width(), windowSize.height()).inverted() * toVector(event->pos(), windowSize)).normalized();
            float distance = mult * 10.0f * event->angleDelta().y() / 120.0f;
            QVector3D move = direction * distance;
            move *= pixelSize(windowSize);
            centerPoint -= rotation.rotatedVector(move);
        }
    } else {
        float zoomNew = this->zoom * std::exp(mult * 0.1f * event->angleDelta().y() / 120.0f);
        zoomNew = std::min(zoomMax, std::max(zoomMin, zoomNew));

        // Change centerPoint so that the object under the mouse stays the same
        // while zooming
        if (true) {
            if (allowMove) {
                QVector3D p(event->pos().x(), -event->pos().y(), 0);
                p -= QVector3D(windowSize.width(), -windowSize.height(), 0) / 2;
                p *= pixelSize(windowSize);
                centerPoint += rotation.rotatedVector(p * (1 - this->zoom / zoomNew));
            }
        }

        this->zoom = zoomNew;
    }
    emit changed();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
