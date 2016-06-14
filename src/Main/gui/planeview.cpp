#include "planeview.hpp"

#include <cmath>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>

#include <QtOpenGL/QGLFormat>

using namespace voxie::gui;
using namespace voxie::data;

PlaneView::PlaneView(Slice *slice, QWidget *parent) :
    OpenGLWidget(parent),
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

QString PlaneView::initialize() {
    if (!vao.create()) {
        return "Creating VAO failed";
        //qWarning() << "Creating VAO failed";
    }

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

    vao.bind();

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

static void push(QVector<GLfloat>& array, const QVector3D& data) {
    array.push_back(data.x());
    array.push_back(data.y());
    array.push_back(data.z());
}

static void push(QVector<GLfloat>& array, const QVector4D& data) {
    array.push_back(data.x());
    array.push_back(data.y());
    array.push_back(data.z());
    array.push_back(data.w());
}

static void addQuad(QVector<GLfloat>& vertices, QVector<GLfloat>& colors,
                    const QVector4D& color,
                    const QVector3D& a, const QVector3D& b,
                    const QVector3D& c, const QVector3D& d) {
    for (int i = 0; i < 6; i++)
        push(colors, color);

    push(vertices, a);
    push(vertices, b);
    push(vertices, c);

    push(vertices, a);
    push(vertices, c);
    push(vertices, d);
}

static void addLine(QVector<GLfloat>& vertices, QVector<GLfloat>& colors,
                    const QVector4D& color,
                    const QVector3D& a, const QVector3D& b) {
    push(colors, color);
    push(colors, color);

    push(vertices, a);
    push(vertices, b);
}

void PlaneView::draw(GLenum mode, const QVector<GLfloat>& vertices, const QVector<GLfloat>& colors, size_t count, const QMatrix4x4& modelMatrix) {
    if (!vertexbuffer.bind()) {
        qCritical() << "Binding vertex buffer failed";
        return;
    }
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    if (!colorbuffer.bind()) {
        qCritical() << "Binding color buffer failed";
        return;
    }
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);

    glUseProgram(program.programId());

    QMatrix4x4 matViewProj = view3d->projectionMatrix(this->width(), this->height()) * view3d->viewMatrix() * modelMatrix;
    glUniformMatrix4fv(MVP_ID, 1, GL_FALSE, matViewProj.constData());

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

    glDrawArrays(mode, 0, count);

    glDisableVertexAttribArray(vertexPosition_modelspaceID);
    glDisableVertexAttribArray(vertexColorID);
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

    QVector<GLfloat> vertices;
    QVector<GLfloat> colors;

    QMatrix4x4 transformDataSet;
    transformDataSet.translate(origin);

    ///////////////////////////////////////////////////////////////////////////////////
    /// Render data set cuboid
    ///////////////////////////////////////////////////////////////////////////////////

    vertices.clear();
    colors.clear();

    // Front
    addQuad(vertices, colors,
            QVector3D(0.8f, 0.8f, 0.8f),
            QVector3D(0,           0,           extends.z()),
            QVector3D(extends.x(), 0,           extends.z()),
            QVector3D(extends.x(), extends.y(), extends.z()),
            QVector3D(0,           extends.y(), extends.z()));

    // Back
    addQuad(vertices, colors,
            QVector3D(0.4f, 0.4f, 0.4f),
            QVector3D(0,           0,           0),
            QVector3D(extends.x(), 0,           0),
            QVector3D(extends.x(), extends.y(), 0),
            QVector3D(0,           extends.y(), 0));

    // Left
    addQuad(vertices, colors,
            QVector3D(0.6f, 0.6f, 0.6f),
            QVector3D(extends.x(), 0,           0),
            QVector3D(extends.x(), 0,           extends.z()),
            QVector3D(extends.x(), extends.y(), extends.z()),
            QVector3D(extends.x(), extends.y(), 0));

    // Right
    addQuad(vertices, colors,
            QVector3D(0.5f, 0.5f, 0.5f),
            QVector3D(0, 0,           0),
            QVector3D(0, 0,           extends.z()),
            QVector3D(0, extends.y(), extends.z()),
            QVector3D(0, extends.y(), 0));

    // Top
    addQuad(vertices, colors,
            QVector3D(0.7f, 0.7f, 0.7f),
            QVector3D(0,           extends.y(), 0),
            QVector3D(0,           extends.y(), extends.z()),
            QVector3D(extends.x(), extends.y(), extends.z()),
            QVector3D(extends.x(), extends.y(), 0));

    // Bottom
    addQuad(vertices, colors,
            QVector3D(0.3f, 0.3f, 0.3f),
            QVector3D(0,           0,  0),
            QVector3D(0,           0, extends.z()),
            QVector3D(extends.x(), 0, extends.z()),
            QVector3D(extends.x(), 0, 0));

    draw(GL_TRIANGLES, vertices, colors, vertices.size() / 3, transformDataSet);


    ///////////////////////////////////////////////////////////////////////////////////
    /// Render coordinate axis
    ///////////////////////////////////////////////////////////////////////////////////

    vertices.clear();
    colors.clear();

    addLine(vertices, colors,
            QVector3D(1.0f, 1.0f, 1.0f),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(2.0f * extends.x(), 0.0f, 0.0f));

    addLine(vertices, colors,
            QVector3D(1.0f, 1.0f, 1.0f),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.0f, 2.0f * extends.y(), 0.0f));

    addLine(vertices, colors,
            QVector3D(1.0f, 1.0f, 1.0f),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.0f, 0.0f, 2.0f * extends.z()));

    draw(GL_LINES, vertices, colors, vertices.size() / 2, QMatrix4x4());

    ///////////////////////////////////////////////////////////////////////////////////
    /// Render plane
    ///////////////////////////////////////////////////////////////////////////////////

    QMatrix4x4 transformPlane;

    // Adjust origin
    //transformPlane.translate(origin);

    // Slice transformation
    transformPlane.translate(this->slice->origin());
    transformPlane.rotate(this->slice->rotation());

    vertices.clear();
    colors.clear();

    addQuad(vertices, colors,
            QVector4D(1.0f, 0.0f, 0.0f, 0.4f),
            QVector3D(-extends.length(), -extends.length(), 0),
            QVector3D(-extends.length(), extends.length(),  0),
            QVector3D( extends.length(), extends.length(),  0),
            QVector3D( extends.length(), -extends.length(), 0));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    draw(GL_TRIANGLES, vertices, colors, vertices.size() / 3, transformPlane);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    vertices.clear();
    colors.clear();

    addLine(vertices, colors,
            QVector3D(0.0f, 1.0f, 0.0f),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(extends.length(), 0.0f, 0.0f));

    addLine(vertices, colors,
            QVector3D(0.0f, 0.0f, 1.0f),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.0f, extends.length(), 0.0f));

    addLine(vertices, colors,
            QVector3D(1.0f, 0.0f, 0.0f),
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.0f, 0.0f, extends.length()));

    draw(GL_LINES, vertices, colors, vertices.size() / 2, transformPlane);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
