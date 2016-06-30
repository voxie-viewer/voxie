#include "isosurfaceview.hpp"

#include <PluginVis3D/sharpthread.hpp>
#include <PluginVis3D/surfaceextractor.hpp>
#include <PluginVis3D/cuberille.hpp>
#include <PluginVis3D/marchingcubes.hpp>

#include <Voxie/data/voxeldata.hpp>
#include <Voxie/data/surfacebuilder.hpp>
#include <Voxie/data/slice.hpp>

#include <Voxie/io/operation.hpp>

#include <math.h>

#include <QtCore/QDebug>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::data;

IsosurfaceView::IsosurfaceView(voxie::data::DataSet *voxelData, QWidget *parent) :
    OpenGLDrawWidget(parent),
    voxelData(voxelData),
    view3d(new voxie::visualization::View3D(this, true, this->voxelData->diagonalSize())),
    threshold(10),
    inverted(false),
    useMarchingCubes(true)
{
    this->resize(500, 400);

    QMetaObject::Connection conni = connect(this->voxelData, &QObject::destroyed, [this]() -> void
    {
        this->voxelData = nullptr;
    });
    connect(this, &QObject::destroyed, [=]() -> void
    {
        this->disconnect(conni);
    });

    QHBoxLayout *hlayout = new QHBoxLayout();
    {
        hlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        QVBoxLayout *vlayout = new QVBoxLayout();
        {
            vlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
            this->progressBar = new QProgressBar();
            this->progressBar->setMinimum(0);
            this->progressBar->setMaximum(1000000);
            this->progressBar->setMaximumSize(250, 30);
            connect(this, &IsosurfaceView::progressChanged, this->progressBar, [this](float value) {
                    this->progressBar->setValue((int) (value * 1000000 + 0.5));
                });
            vlayout->addWidget(this->progressBar);
            vlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        }
        hlayout->addLayout(vlayout);
        hlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    }
    this->setLayout(hlayout);

    connect(view3d, &voxie::visualization::View3D::changed, this, [this] { this->repaint(); });

    qRegisterMetaType<QSharedPointer<voxie::data::Surface>>();

    connect(this, &IsosurfaceView::generationDone, this, &IsosurfaceView::updateSurface);

    connect(this->voxelData, &DataSet::sliceCreated, this, [this](Slice* slice){ this->addSlice(slice, true); });
    for (auto slice : this->voxelData->getSlices()) {
        addSlice(slice, false);
    }

    highlightTimer.setSingleShot(true);
    connect(&highlightTimer, &QTimer::timeout, this, &IsosurfaceView::highlightTimeout);
}

IsosurfaceView::~IsosurfaceView() {
}

void IsosurfaceView::addSlice(Slice* slice, bool changedNow) {
    connect(slice, &Slice::planeChanged, this, [this](Plane oldPlane, Plane newPlane, bool equivalent) {
            Q_UNUSED(oldPlane);
            Q_UNUSED(equivalent);
            updateSlice(newPlane);
        });
    if (changedNow)
        updateSlice(slice->getCuttingPlane());
}
void IsosurfaceView::updateSlice(const Plane& newPlane) {
    //qDebug() << newPlane.origin << newPlane.rotation;

    highlightTimer.start(2000);
    highlightedPlane = newPlane;
    hasHighlightedPlane = true;
    update();
}
void IsosurfaceView::highlightTimeout() {
    //qDebug() << "Timeout";

    hasHighlightedPlane = false;
    update();
}

QString IsosurfaceView::initialize() {
    QString error = OpenGLDrawWidget::initialize();
    if (error != "")
        return error;

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
        "attribute vec3 vertexColor;\n"
        "\n"
        "varying vec4 fragmentColor;\n"
        "\n"
        "uniform mat4 MVP;\n"
        "\n"
        "void main() {\n"
        "    gl_Position = MVP * vec4(vertexPosition_modelspace,1);\n"
        "    fragmentColor = vec4(vertexColor, 1);\n"
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

    if (!surfaceVertexBuffer.create()) {
        return "Surface vertex buffer create failed";
    }

    if (!surfaceColorBuffer.create()) {
        return "Surface color buffer create failed";
    }

    return "";
}

void IsosurfaceView::regenerate()
{
    if(this->voxelData == nullptr)
        return;
    if (generating) {
        generationRequested = true;
        return;
    }

    SharpThread *thread = new SharpThread([this]() -> void { this->generateModel(); }, this);

    this->progressBar->setValue(0);

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this->progressBar, &QWidget::hide);
    connect(thread, &QThread::started, this->progressBar, &QWidget::show);

    this->generating = true;
    thread->start();
}

void IsosurfaceView::generateModel()
{
    qDebug() << "Regenrating surface...";

    QScopedPointer<voxie::io::Operation> operation(new voxie::io::Operation());
    connect(operation.data(), &voxie::io::Operation::progressChanged, this, &IsosurfaceView::progressChanged);

    QScopedPointer<SurfaceExtractor> extractor;
    if (!this->useMarchingCubes) {
        extractor.reset(new Cuberille());
    } else {
        extractor.reset(new MarchingCubes());
    }

    auto surface = extractor->extract(operation.data(), this->voxelData->filteredData(), this->threshold, this->inverted);

    qDebug() << "Generated surface: Vertices:" << surface->vertices().size() << "/ Triangles:" << surface->triangles().size();

    emit generationDone(surface);
}

void IsosurfaceView::updateSurface(const QSharedPointer<voxie::data::Surface>& surface) {
    this->generating = false;
    this->surface = surface;
    this->uploadData();
    this->update();
    if (generationRequested) {
        generationRequested = false;
        regenerate();
    }
}

// Like QVector3D::normalized(), but with a smaller minimum size
QVector3D IsosurfaceView::normalized(const QVector3D& value) {
    double absSquared = double(value.x()) * double(value.x()) +
        double(value.y()) * double(value.y()) +
        double(value.z()) * double(value.z());
    if (absSquared >= 1e-50) { // minimum length 1e-25
        double len = std::sqrt(absSquared);
        return QVector3D(float(double(value.x()) / len),
                         float(double(value.y()) / len),
                         float(double(value.z()) / len));
    } else {
        return QVector3D();
    }
}

void IsosurfaceView::uploadData() {
    // TODO: Use indexed mode?
    // TODO: Do normal / light calculation in shader?

    vertexCount = 0;

    this->makeCurrent();

    struct Light {
        QVector3D dir;
        QVector3D color;
    };

    Light lights[] = {
        {
            QVector3D(0.7f, -1.0f, 0.4f).normalized(),
            //{ 0.8f, 0.8f, 1.0f }
            { 1.0f, 1.0f, 1.0f }
        },
        {
            QVector3D(-0.4f, 1.0f, -0.7f).normalized(),
            { 1.0f, 1.0f, 0.8f }
        }
    };

    QVector<GLfloat> vertices;
    QVector<GLfloat> colors;

    for (int i = 0; i < surface->triangles().size(); i++) {
        const auto& triangle = surface->triangles()[i];
        QVector3D a = surface->vertices()[triangle[0]];
        QVector3D b = surface->vertices()[triangle[1]];
        QVector3D c = surface->vertices()[triangle[2]];

        QVector3D normal = normalized(QVector3D::crossProduct(b - c, c - a));
 
        // TODO: What to do with very small triangles where the cross product is almost zero?
        /*
          if (normal == QVector3D(0,0,0))
          qDebug()<<"X"<<triangles[i].p[0]<<triangles[i].p[1]<<triangles[i].p[2];
        */

        QVector3D color(0.1f, 0.1f, 0.1f);

        for(size_t i = 0; i < (sizeof(lights) / sizeof(Light)); i++) {
            float lighting = fmax(0.0f, QVector3D::dotProduct(normal, lights[i].dir));
            color += 0.5f * lighting * lights[i].color;
        }

        /*
        QVector4D colorA(color.x(), color.y(), color.z(), 1.0f);
        PrimitiveBuffer::push(colors, colorA);
        PrimitiveBuffer::push(colors, colorA);
        PrimitiveBuffer::push(colors, colorA);
        */
        PrimitiveBuffer::push(colors, color);
        PrimitiveBuffer::push(colors, color);
        PrimitiveBuffer::push(colors, color);

        PrimitiveBuffer::push(vertices, a);
        PrimitiveBuffer::push(vertices, b);
        PrimitiveBuffer::push(vertices, c);
    }

    if (!surfaceVertexBuffer.bind()) {
        qCritical() << "Binding surface vertex buffer failed";
        vertexCount = 0;
        this->doneCurrent();
        return;
    }
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    if (!surfaceColorBuffer.bind()) {
        qCritical() << "Binding surface color buffer failed";
        vertexCount = 0;
        this->doneCurrent();
        return;
    }
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);

    vertexCount = vertices.size() / 3;

    this->doneCurrent();
}

void IsosurfaceView::paint() {
    QMatrix4x4 matViewProj = view3d->projectionMatrix(this->width(), this->height()) * view3d->viewMatrix();
    paint(matViewProj);
}

void IsosurfaceView::paint(const QMatrix4x4& matViewProj) {
    // Create new vao to make sure it is valid for the current context
    QOpenGLVertexArrayObject vao;
    if (!vao.create()) {
        //return "Creating VAO failed";
        qWarning() << "Creating VAO failed";
    }

    QColor color = this->palette().background().color();

    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!voxelData)
        return;

    if (surface && vertexCount != 0) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        switch (culling) {
        case SHOW_FRONT:
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CCW);
            break;
        case SHOW_BACK:
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CW);
            break;
        default:
            glDisable(GL_CULL_FACE);
            break;
        }

        glPointSize(1.0f);

        glUseProgram(program.programId());

        glUniformMatrix4fv(MVP_ID, 1, GL_FALSE, matViewProj.constData());

        vao.bind();

        // 1st attribute buffer : vertices
        glEnableVertexAttribArray(vertexPosition_modelspaceID);
        if (!surfaceVertexBuffer.bind()) {
            qCritical() << "Binding surface vertex buffer failed";
            return;
        }
        glVertexAttribPointer(vertexPosition_modelspaceID,  // attribute
                              3,                  // size
                              GL_FLOAT,           // type
                              GL_FALSE,           // normalized?
                              0,                  // stride
                              (void*)0            // array buffer offset
                              );
        surfaceVertexBuffer.release();

        // 2nd attribute buffer : colors
        glEnableVertexAttribArray(vertexColorID);
        if (!surfaceColorBuffer.bind()) {
            qCritical() << "Binding surface color buffer failed";
            return;
        }
        glVertexAttribPointer(vertexColorID,  // attribute
                              3,              // size
                              GL_FLOAT,       // type
                              GL_FALSE,       // normalized?
                              0,              // stride
                              (void*)0        // array buffer offset
                              );
        surfaceColorBuffer.release();

        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glDisableVertexAttribArray(vertexPosition_modelspaceID);
        glDisableVertexAttribArray(vertexColorID);

        vao.release();

        glUseProgram(0);
    }

    if (hasHighlightedPlane) {
        //qDebug() << highlightedPlane.origin << highlightedPlane.rotation;

        float volumeDiag = this->voxelData->diagonalSize();
        float planeSize = volumeDiag / 3;
        float axisLength = volumeDiag / 3;

        PrimitiveBuffer buffer;

        glDisable(GL_CULL_FACE);

        ///////////////////////////////////////////////////////////////////////////////////
        /// Render plane
        ///////////////////////////////////////////////////////////////////////////////////

        QMatrix4x4 transformPlane = matViewProj;

        // Adjust origin
        //transformPlane.translate(origin);

        // Slice transformation
        transformPlane.translate(highlightedPlane.origin);
        transformPlane.rotate(highlightedPlane.rotation);

        buffer.clear();

        buffer.addQuad(QVector4D(1.0f, 0.0f, 0.0f, 0.4f),
                       QVector3D(-planeSize, -planeSize, 0),
                       QVector3D(-planeSize, planeSize,  0),
                       QVector3D( planeSize, planeSize,  0),
                       QVector3D( planeSize, -planeSize, 0));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        draw(buffer, transformPlane);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        buffer.clear();

        buffer.addLine(QVector3D(0.0f, 1.0f, 0.0f),
                       QVector3D(0.0f, 0.0f, 0.0f),
                       QVector3D(axisLength, 0.0f, 0.0f));

        buffer.addLine(QVector3D(0.0f, 0.0f, 1.0f),
                       QVector3D(0.0f, 0.0f, 0.0f),
                       QVector3D(0.0f, axisLength, 0.0f));

        buffer.addLine(QVector3D(1.0f, 0.0f, 0.0f),
                       QVector3D(0.0f, 0.0f, 0.0f),
                       QVector3D(0.0f, 0.0f, axisLength));

        draw(buffer, transformPlane);
    }
}


void IsosurfaceView::mousePressEvent(QMouseEvent *event)
{
    view3d->mousePressEvent(mouseLast, event, size());
	this->mouseLast = event->pos();
}

void IsosurfaceView::mouseMoveEvent(QMouseEvent *event)
{
    view3d->mouseMoveEvent(mouseLast, event, size());
	this->mouseLast = event->pos();
}

void IsosurfaceView::wheelEvent(QWheelEvent *event)
{
    view3d->wheelEvent(event, size());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
