#include "openglwidget.hpp"

#include <QDebug>

using namespace voxie::visualization;

static QString getGLErrorString(GLenum error) {
    switch (error) {
    case GL_NO_ERROR: return "GL_NO_ERROR";
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    default: return QString::number(error);
    }
}
bool OpenGLWidget::checkOpenGLStatus() {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return true;

    QString errorStr;
    int count = 0;
    while (error != GL_NO_ERROR) {
        if (count != 0)
            errorStr += ",";
        errorStr += getGLErrorString(error);

        if (count > 20)
            break; // Prevent endless loop if glGetError() doesn't clear the error

        error = glGetError();
        count++;
    }
    qWarning() << "Got OpenGL error:" << errorStr;
    return false;
}

OpenGLWidget::OpenGLWidget(QWidget *parent) :
	QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::Rgba), parent),
	fWidth(1), fHeight(1)
{
}


void OpenGLWidget::initializeGL() {
#define FAIL(x) do {                            \
        initError = x;                          \
        qCritical() << x;                       \
        return;                                 \
    } while (0)

    if (!context()->isValid()) {
        FAIL("OpenGL context is not valid");
    }

    /*
    auto logger = new QOpenGLDebugLogger(this);
    logger->initialize();
    connect(logger, &QOpenGLDebugLogger::messageLogged, [] (const QOpenGLDebugMessage& msg) { qWarning() << "OpenGL Debug message:" << msg; });
    logger->startLogging();
    */

    initializeOpenGLFunctions(); // QOpenGLFunctions::initializeOpenGLFunctions() returns a void
    /*
    if (!initializeOpenGLFunctions()) {
        FAIL("Could not initialize OpenGL functions");
    }
    */

    QString error = initialize();
    if (error != "")
        FAIL(error);

    if (!checkOpenGLStatus())
        FAIL("OpenGL error during initialization");

    initialized = true;

#undef FAIL
}

void OpenGLWidget::resizeGL(int w, int h) {
	this->fWidth = static_cast<float>(w);
	this->fHeight = static_cast<float>(h);

	this->fHeight = std::max<float>(this->fHeight, 1);

    if (!initialized)
        return;

	glViewport(0, 0, w, h);
}

void OpenGLWidget::paintGL() {
    if (!initialized)
        return;

    paint();
}

void OpenGLWidget::paintEvent(QPaintEvent *event) {
    if (initialized) {
        checkOpenGLStatus();
        QGLWidget::paintEvent(event);
        checkOpenGLStatus();
        return;
    }

    QPainter painter;
    painter.begin(this);
    painter.drawText(QRect(QPoint(0, 0), this->size()), Qt::AlignCenter, "Error initializing OpenGL:\n" + initError);
    painter.end();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
