#pragma once

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFunctions_3_2_Compatibility>

#include <QtOpenGL/QGLWidget>

#include <osvr/RenderKit/RenderManager.h>

namespace osvr {
    namespace clientkit {
        class ClientContext;
    }
    namespace renderkit {
        class RenderManager;
    }
}

class Qt5ToolkitImpl;

class OsvrDisplay : public QObject, public QOpenGLFunctions_3_2_Compatibility {
    Q_OBJECT

    QOpenGLContext* context;

    QSharedPointer<osvr::clientkit::ClientContext> osvrContext;
    QSharedPointer<osvr::renderkit::RenderManager> renderManager;

    std::vector<osvr::renderkit::RenderBuffer> colorBuffers;
    std::vector<GLuint> depthBuffers;

    GLuint frameBuffer;

    Qt5ToolkitImpl* toolkit;

    QTimer* timer;

    void updateDisplay();

public:
    OsvrDisplay(QOpenGLContext* context, QGLWidget* shareWidget, QObject* parent);
    ~OsvrDisplay();

    void resetYaw();

signals:
    void render(const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
