#pragma once

#include <cmath>
#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QObject>

#include <QtGui/QMouseEvent>
#include <QtGui/QQuaternion>

namespace voxie {

namespace spnav {
class SpaceNavVisualizer;
}

namespace visualization {

class VOXIECORESHARED_EXPORT View3D : public QObject {
    Q_OBJECT

    // minimal and maximal values for zoom factor
    float zoomMin, zoomMax;

    // whether moving the centerPoint is allowed
    bool allowMove;

    // Field of view in y direction in rad (does not change)
    float fieldOfView;

    // Size of region (in y direction) which can be seen in the centerPoint
    // plane on standard zoom
    float viewSizeStandard;

    // Zoom factor
    float zoom;

    float viewSize() const {
        //return viewSizeStandard / zoom;
        return viewSizeStandard;
    }

    float pixelSize(const QSize& windowSize) const {
        // Size of 1 pixel at the centerPoint in m
        return viewSize() / windowSize.height() / this->zoom;
    }

    // Distance from centerPoint to camera
    float distanceCameraCenter() const {
        // 1/2 * viewSize * cot(fieldOfView/2)
        return 0.5f * viewSize() * std::cos(fieldOfView/2) / std::sin(fieldOfView/2);
    }

    // Coordiantes (in world space) around which rotations will happen
    QVector3D centerPoint;

    // Rotation which will transform coordinates from eye space to world space
    // (i.e. the rotation of the camera)
    QQuaternion rotation;

public:
    View3D(QObject* parent, bool allowMove, float viewSizeStandard, float zoomMin = 1e-10, float zoomMax = 1e+10);

    QMatrix4x4 viewMatrix();
    QMatrix4x4 projectionMatrix(float fWidth, float fHeight);

    void mousePressEvent(const QPoint& mouseLast, QMouseEvent *event, const QSize& windowSize);
    void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent *event, const QSize& windowSize);
    void wheelEvent(QWheelEvent *event, const QSize& windowSize);

    void move(QVector3D vectorViewSpace);
    void rotate(QQuaternion rotation);
    void moveZoom(float value);

    void registerSpaceNavVisualizer(voxie::spnav::SpaceNavVisualizer* sn);

    const QVector3D& getCenterPoint() const { return centerPoint; }

signals:
    void changed();
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
