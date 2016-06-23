#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLFunctions>

#include <QtOpenGL/QGLWidget>

namespace voxie
{
namespace visualization
{

class VOXIECORESHARED_EXPORT OpenGLWidget : public QGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    float fWidth, fHeight;

    bool initialized = false;
    QString initError;

public:
    explicit OpenGLWidget(QWidget *parent = 0);

    float width() const { return fWidth; }
    float height() const { return fHeight; }

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

    virtual void paintEvent(QPaintEvent *event) override;

    virtual QString initialize() = 0;
    virtual void paint() = 0;

    bool checkOpenGLStatus();
};

// Subclass of OpenGLWidget which provides some basic 3D drawing operations
class VOXIECORESHARED_EXPORT OpenGLDrawWidget : public OpenGLWidget {
    Q_OBJECT

private:
    QOpenGLVertexArrayObject vao;
    QOpenGLShaderProgram program;
    GLuint MVP_ID;
    GLuint vertexPosition_modelspaceID;
    GLuint vertexColorID;
    QOpenGLBuffer vertexbuffer;
    QOpenGLBuffer colorbuffer;

public:
    explicit OpenGLDrawWidget(QWidget *parent = 0);

    virtual QString initialize();

    class VOXIECORESHARED_EXPORT PrimitiveBuffer {
    public:
        QVector<GLfloat> vertices;
        QVector<GLfloat> colors;
        GLenum mode;

        static void push(QVector<GLfloat>& array, const QVector3D& data);
        static void push(QVector<GLfloat>& array, const QVector4D& data);

        // Adds two triangles
        void addQuad(const QVector4D& color,
                     const QVector3D& a, const QVector3D& b,
                     const QVector3D& c, const QVector3D& d);

        void addLine(const QVector4D& color,
                     const QVector3D& a, const QVector3D& b);

        void clear();
    };

    /**
     * Draw primitives. The vertices are taken from the vertices vector,
     * (3 values per vertex), the colors from the colors vector (4 values per
     * vertex, including alpha value).
     */
    void draw(GLenum mode, const QVector<GLfloat>& vertices, const QVector<GLfloat>& colors, const QMatrix4x4& modelViewProjectionMatrix);

    void draw(const PrimitiveBuffer& buffer, const QMatrix4x4& modelViewProjectionMatrix);
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
