#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QObject>

#include <QtGui/QMouseEvent>

namespace voxie {

namespace visualization {

class VOXIECORESHARED_EXPORT View3D : public QObject {
    Q_OBJECT

    float zoomMin, zoomMax;

public:
    float pan, tilt, zoom;

    View3D(QObject* parent, float zoomMin = -1e+30, float zoomMax = 1e+30);

    QMatrix4x4 viewMatrix(float scaling);
    QMatrix4x4 projectionMatrix(float scaling, float fWidth, float fHeight);

    QVector3D cameraPosition(float scaling);

    void mousePressEvent(const QPoint& mouseLast, QMouseEvent *event);
    void mouseMoveEvent(const QPoint& mouseLast, QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

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
