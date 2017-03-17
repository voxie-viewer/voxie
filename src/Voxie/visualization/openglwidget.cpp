#include "openglwidget.hpp"

#include <Voxie/ivoxie.hpp>

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

    if (voxieRoot().disableOpenGL()) {
        FAIL("OpenGL support is disabled from the command line");
    }

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

    initialized_ = true;

#undef FAIL
}

void OpenGLWidget::resizeGL(int w, int h) {
	this->fWidth = static_cast<float>(w);
	this->fHeight = static_cast<float>(h);

	this->fHeight = std::max<float>(this->fHeight, 1);

    if (!initialized())
        return;

	glViewport(0, 0, w, h);
}

void OpenGLWidget::paintGL() {
    if (!initialized())
        return;

    paint();
}

void OpenGLWidget::paintEvent(QPaintEvent *event) {
    if (initialized()) {
        checkOpenGLStatus();
        QGLWidget::paintEvent(event);
        checkOpenGLStatus();
        return;
    }

    //return;
    QPainter painter;
    painter.begin(this);
    painter.drawText(QRect(QPoint(0, 0), this->size()), Qt::AlignCenter, "Error initializing OpenGL:\n" + initError);
    painter.end();
}

OpenGLDrawWidget::OpenGLDrawWidget(QWidget *parent) : OpenGLWidget(parent) {
}

QString OpenGLDrawWidget::initialize() {
    /*
    if (!vao.create()) {
        //return "Creating VAO failed";
        qWarning() << "Creating VAO failed";
    }
    */

    const char* vshader =
        "#version 110\n"
        "\n"
        "attribute vec3 vertexPosition_modelspace;\n"
        "attribute vec4 vertexColor;\n"
        "\n"
        "varying vec4 fragmentColor;\n"
        "\n"
        "uniform mat4 MVP;\n"
        "\n"
        "void main() {\n"
        "    gl_Position = MVP * vec4(vertexPosition_modelspace,1);\n"
        "    fragmentColor = vertexColor;\n"
        "}\n"
        ;
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vshader)) {
        return "Compiling vertex shader failed";
    }
    
    const char* fshader =
        "#version 110\n"
        "\n"
        "varying vec4 fragmentColor;\n"
        "\n"
        "void main(){\n"
        "\n"
        "    gl_FragColor = fragmentColor;\n"
        "\n"
        "}\n"
        ;
    if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, fshader)) {
        return "Compiling fragment shader failed";
    }

    if (!program.link()) {
        return "Linking shaders failed";
    }

    MVP_ID = glGetUniformLocation(program.programId(), "MVP");
    
    vertexPosition_modelspaceID = glGetAttribLocation(program.programId(), "vertexPosition_modelspace");
    vertexColorID = glGetAttribLocation(program.programId(), "vertexColor");

    if (!program.bind()) {
        return "Binding shaders failed";
    }

    if (!vertexbuffer.create()) {
        return "Vertex buffer create failed";
    }

    if (!colorbuffer.create()) {
        return "Color buffer create failed";
    }

    return "";
}

void OpenGLDrawWidget::PrimitiveBuffer::push(std::vector<GLfloat>& array, const QVector3D& data) {
    array.push_back(data.x());
    array.push_back(data.y());
    array.push_back(data.z());
}

void OpenGLDrawWidget::PrimitiveBuffer::push(std::vector<GLfloat>& array, const QVector4D& data) {
    array.push_back(data.x());
    array.push_back(data.y());
    array.push_back(data.z());
    array.push_back(data.w());
}

void OpenGLDrawWidget::PrimitiveBuffer::addQuad(
                    const QVector4D& color,
                    const QVector3D& a, const QVector3D& b,
                    const QVector3D& c, const QVector3D& d) {
    if (vertices.size() == 0) {
        mode = GL_TRIANGLES;
    } else if (mode != GL_TRIANGLES) {
        qCritical() << "OpenGLDrawWidget::PrimitiveBuffer::addQuad(): Buffer does not contains triangles";
        return;
    }

    for (int i = 0; i < 6; i++)
        push(colors, color);

    push(vertices, a);
    push(vertices, b);
    push(vertices, c);

    push(vertices, a);
    push(vertices, c);
    push(vertices, d);
}

void OpenGLDrawWidget::PrimitiveBuffer::addLine(
                    const QVector4D& color,
                    const QVector3D& a, const QVector3D& b) {
    if (vertices.size() == 0) {
        mode = GL_LINES;
    } else if (mode != GL_LINES) {
        qCritical() << "OpenGLDrawWidget::PrimitiveBuffer::addQuad(): Buffer does not contains triangles";
        return;
    }

    push(colors, color);
    push(colors, color);

    push(vertices, a);
    push(vertices, b);
}

void OpenGLDrawWidget::PrimitiveBuffer::clear() {
    vertices.clear();
    colors.clear();
}

void OpenGLDrawWidget::draw(GLenum mode, const std::vector<GLfloat>& vertices, const std::vector<GLfloat>& colors, const QMatrix4x4& modelViewProjectionMatrix) {
    // Create new vao to make sure it is valid for the current context
    QOpenGLVertexArrayObject vao;
    if (!vao.create()) {
        //return "Creating VAO failed";
        qWarning() << "Creating VAO failed";
    }

    vao.bind();

    if (!vertexbuffer.bind()) {
        qCritical() << "Binding vertex buffer failed";
        return;
    }
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    vertexbuffer.release();

    if (!colorbuffer.bind()) {
        qCritical() << "Binding color buffer failed";
        return;
    }
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
    colorbuffer.release();

    glUseProgram(program.programId());

    glUniformMatrix4fv(MVP_ID, 1, GL_FALSE, modelViewProjectionMatrix.constData());

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(vertexPosition_modelspaceID);
    if (!vertexbuffer.bind()) {
        qCritical() << "Binding vertex buffer failed";
        return;
    }
    glVertexAttribPointer(vertexPosition_modelspaceID,  // attribute
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(vertexColorID);
    if (!colorbuffer.bind()) {
        qCritical() << "Binding color buffer failed";
        return;
    }
    glVertexAttribPointer(vertexColorID,  // attribute
                          4,              // size
                          GL_FLOAT,       // type
                          GL_FALSE,       // normalized?
                          0,              // stride
                          (void*)0        // array buffer offset
                          );

    glDrawArrays(mode, 0, vertices.size() / 3);

    glDisableVertexAttribArray(vertexPosition_modelspaceID);
    glDisableVertexAttribArray(vertexColorID);

    vao.release();
}

void OpenGLDrawWidget::draw(const PrimitiveBuffer& buffer, const QMatrix4x4& modelViewProjectionMatrix) {
    if (buffer.vertices.size() == 0)
        return;

    draw(buffer.mode, buffer.vertices, buffer.colors, modelViewProjectionMatrix);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
