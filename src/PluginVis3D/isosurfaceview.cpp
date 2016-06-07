#include "isosurfaceview.hpp"

#include <PluginVis3D/marchingcubes.hpp>
#include <PluginVis3D/sharpthread.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <math.h>

#include <QtCore/QDebug>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::data;

IsosurfaceView::IsosurfaceView(voxie::data::DataSet *voxelData, QWidget *parent) :
    QGLWidget(parent),
    voxelData(voxelData),
    fWidth(1.0f), fHeight(1.0f),
    lists(),
    view3d(new voxie::visualization::View3D(this)),
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
            this->progressBar->setMaximumSize(250, 30);
            connect(this, &IsosurfaceView::progressChanged, this->progressBar, &QProgressBar::setValue);
            vlayout->addWidget(this->progressBar);
            vlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        }
        hlayout->addLayout(vlayout);
        hlayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    }
    this->setLayout(hlayout);

    connect(view3d, &voxie::visualization::View3D::changed, this, [this] { this->repaint(); });
}

void IsosurfaceView::genCube(const QVector3D &position, int sides)
{
    QVector3D spacing = this->voxelData->filteredData()->getSpacing();

    QVector3D size = 0.5f * spacing;

    QVector3D pos;
    pos.setX(position.x() * spacing.x());
    pos.setY(position.y() * spacing.y());
    pos.setZ(position.z() * spacing.z());

    QVector3D min = pos - size;
    QVector3D max = pos + size;

    // Front (+z)
    if(sides & 1)
    {
        glColor3f(0.8f, 0.8f, 0.8f);
        glVertex3f(min.x(), min.y(), max.z());
        glVertex3f(max.x(), min.y(), max.z());
        glVertex3f(max.x(), max.y(), max.z());
        glVertex3f(min.x(), max.y(), max.z());
    }

    // Back (-z)
    if(sides & 2)
    {
        glColor3f(0.4f, 0.4f, 0.4f);
        glVertex3f(min.x(), min.y(), min.z());
        glVertex3f(max.x(), min.y(), min.z());
        glVertex3f(max.x(), max.y(), min.z());
        glVertex3f(min.x(), max.y(), min.z());
    }

    // Left (+x)
    if(sides & 4)
    {
        glColor3f(0.6f, 0.6f, 0.6f);
        glVertex3f(max.x(), min.y(), min.z());
        glVertex3f(max.x(), min.y(), max.z());
        glVertex3f(max.x(), max.y(), max.z());
        glVertex3f(max.x(), max.y(), min.z());
    }

    // Right (-x)
    if(sides & 8)
    {
        glColor3f(0.5f, 0.5f, 0.5f);
        glVertex3f(min.x(), min.y(), min.z());
        glVertex3f(min.x(), min.y(), max.z());
        glVertex3f(min.x(), max.y(), max.z());
        glVertex3f(min.x(), max.y(), min.z());
    }

    // Top (+y)
    if(sides & 16)
    {
        glColor3f(0.7f, 0.7f, 0.7f);
        glVertex3f(min.x(), max.y(), min.z());
        glVertex3f(min.x(), max.y(), max.z());
        glVertex3f(max.x(), max.y(), max.z());
        glVertex3f(max.x(), max.y(), min.z());
    }

    // Bottom (-y)
    if(sides & 32)
    {
        glColor3f(0.3f, 0.3f, 0.3f);
        glVertex3f(min.x(), min.y(), min.z());
        glVertex3f(min.x(), min.y(), max.z());
        glVertex3f(max.x(), min.y(), max.z());
        glVertex3f(max.x(), min.y(), min.z());
    }
}

void IsosurfaceView::initializeGL()
{
}

void IsosurfaceView::regenerate()
{
    if(this->voxelData == nullptr)
        return;

    SharpThread *threat = new SharpThread([this]() -> void { this->generateModel(); }, this);

    connect(threat, &QThread::finished, threat, &QObject::deleteLater);
    connect(threat, &QThread::finished, this->progressBar, &QWidget::hide);
    connect(threat, &QThread::started, this->progressBar, &QWidget::show);

    this->generating = true;
    this->context()->doneCurrent();
    this->context()->moveToThread(threat);
    threat->start();
}

void IsosurfaceView::generateModel()
{
    qDebug() << "Regenrating...";
    this->makeCurrent();

    for(GLuint list : this->lists)
    {
        glDeleteLists(list, 1);
    }
    this->lists.clear();

    size_t x, y, z;
    voxie::scripting::IntVector3 dim = this->voxelData->filteredData()->getDimensions();
    voxie::scripting::IntVector3 upper = dim;
    upper.x -= 1;
    upper.y -= 1;
    upper.z -= 1;

    const size_t voxelPerList = 250000;

    this->progressBar->setMinimum(0);
    this->progressBar->setMaximum(dim.x - 1);

    QVector3D spacing = this->voxelData->filteredData()->getSpacing();
    QVector3D origin = this->voxelData->filteredData()->getFirstVoxelPosition();

    struct Light
    {
        QVector3D dir;
        QVector3D color;
    };

    Light lights[] =
    {
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

    static const QVector3D offsets[] =
    {
        QVector3D(0, 0, 1),
        QVector3D(1, 0, 1),
        QVector3D(1, 0, 0),
        QVector3D(0, 0, 0),
        QVector3D(0, 1, 1),
        QVector3D(1, 1, 1),
        QVector3D(1, 1, 0),
        QVector3D(0, 1, 0)
    };

    bool regen = true;
    size_t counter = 0;
    for(x = 0; x < dim.x; x++)
    {
        for(y = 0; y < dim.y; y++)
        {
            for(z = 0; z < dim.z; z++)
            {
                if(this->voxelData == nullptr)
                    return;
                Voxel voxel = this->voxelData->filteredData()->getVoxel(x, y, z);
                if(this->useMarchingCubes == false)
                {
                    if((voxel < this->threshold) ^ this->inverted)
                        continue;

                    int majoraMask = 0xFF;
                    if((x > 0) && ((this->voxelData->filteredData()->getVoxel(x-1,y,z) >= this->threshold) ^ this->inverted))
                    {
                        majoraMask &= ~8;
                    }
                    if((y > 0) && ((this->voxelData->filteredData()->getVoxel(x,y-1,z) >= this->threshold) ^ this->inverted))
                    {
                        majoraMask &= ~32;
                    }
                    if((z > 0) && ((this->voxelData->filteredData()->getVoxel(x,y,z-1) >= this->threshold) ^ this->inverted))
                    {
                        majoraMask &= ~2;
                    }
                    if((x < upper.x) && ((this->voxelData->filteredData()->getVoxel(x+1,y,z) >= this->threshold) ^ this->inverted))
                    {
                        majoraMask &= ~4;
                    }
                    if((y < upper.y) && ((this->voxelData->filteredData()->getVoxel(x,y+1,z) >= this->threshold) ^ this->inverted))
                    {
                        majoraMask &= ~16;
                    }
                    if((z < upper.z) && ((this->voxelData->filteredData()->getVoxel(x,y,z+1) >= this->threshold) ^ this->inverted))
                    {
                        majoraMask &= ~1;
                    }

                    if((majoraMask & 192) != 0)
                    {
                        if(counter == 0 && regen)
                        {
                            GLuint list = glGenLists(1);

                            this->lists.append(list);

                            glNewList(list, GL_COMPILE);
                            glPointSize(1.0f);
                            glBegin(GL_QUADS);
                            regen = false;
                        }
                        glColor3f(voxel, voxel, voxel);
                        QVector3D pos(x,y,z);
                        this->genCube(pos, majoraMask);

                        counter++;
                    }
                }
                else
                {
                    ::TRIANGLE triangles[64];
                    ::GRIDCELL cell;

                    QVector3D pos = QVector3D(
                                x * spacing.x(),
                                y * spacing.y(),
                                z * spacing.z()) - 0.5f * spacing;

                    for(int i = 0; i < 8; i++)
                    {
                        cell.p[i] = pos + offsets[i] * spacing + origin;
                        cell.val[i] = this->voxelData->filteredData()->getVoxelMetric(
                                    cell.p[i].x(),
                                    cell.p[i].y(),
                                    cell.p[i].z(),
                                    InterpolationMethod::linear);
                        if(this->inverted)
                        {
                            cell.val[i] = -(cell.val[i] - this->threshold) + this->threshold;
                        }
                        if(isnan(cell.val[i]))
                            cell.val[i] = 0.5 * this->threshold;
                    }
                    int count = Polygonise(cell, this->threshold, triangles, false);
                    if(count == 0)
                        continue;

                    if(counter == 0 && regen)
                    {
                        GLuint list = glGenLists(1);

                        this->lists.append(list);

                        glNewList(list, GL_COMPILE);
                        glPointSize(1.0f);
                        glBegin(GL_TRIANGLES);
                        regen = false;
                    }

                    for(int i = 0; i < count; i++)
                    {
                        QVector3D _a = (triangles[i].p[1] - triangles[i].p[0]).normalized();
                        QVector3D _b = (triangles[i].p[2] - triangles[i].p[0]).normalized();

                        QVector3D normal = QVector3D::crossProduct(_a, _b).normalized();

                        // TODO: What to do with very small triangles where the cross product is almost zero?
                        /*
                        if (normal == QVector3D(0,0,0))
                            qDebug()<<"X"<<triangles[i].p[0]<<triangles[i].p[1]<<triangles[i].p[2];
                        */

                        QVector3D color(0.1f, 0.1f, 0.1f);

                        for(size_t i = 0; i < (sizeof(lights) / sizeof(Light)); i++)
                        {
                            float lighting = fmax(0.0f, QVector3D::dotProduct(normal, lights[i].dir));
                            color += 0.5f * lighting * lights[i].color;
                        }

                        glColor3f(color.x(), color.y(), color.z());

                        for(int j = 0; j < 3; j++)
                        {
                            glVertex3f(
                                triangles[i].p[j].x() - origin.x(),
                                triangles[i].p[j].y() - origin.x(),
                                triangles[i].p[j].z() - origin.x());
                        }

                        counter += 50;
                    }
                }
                if(counter >= voxelPerList)
                {
                    glEnd();
                    glEndList();
                    counter = 0;
                    regen = true;
                    qDebug() << "Generated list:" << this->lists.size();
                }
            }
            emit this->progressChanged(x);
        }
    }

    if(counter > 0)
    {
        glEnd();
        glEndList();
    }

    qDebug() << "Generated" << this->lists.size() << "lists.";

    this->context()->doneCurrent();
    this->context()->moveToThread(this->thread());

    this->generating = false;
}

void IsosurfaceView::glInit()
{
    if(this->generating == true)
        return;
    QGLWidget::glInit();
}

void IsosurfaceView::glDraw()
{
    if(this->generating == true)
        return;
    QGLWidget::glDraw();
}

void IsosurfaceView::resizeEvent(QResizeEvent *event)
{
    if(this->generating == true)
        return;
    QGLWidget::resizeEvent(event);
}

void IsosurfaceView::paintEvent(QPaintEvent *event)
{
    if(this->generating == true)
    {
        return;
    }
    else
    {
        QGLWidget::paintEvent(event);
    }
}

void IsosurfaceView::resizeGL(int w, int h)
{
    this->fWidth = static_cast<float>(w);
    this->fHeight = static_cast<float>(h);
    this->fHeight = std::max<float>(this->fHeight, 1);
}

void IsosurfaceView::paintGL()
{
    if(this->generating)
    {
        // Render nothing here
        return;
    }
    glViewport(0, 0, static_cast<int>(this->fWidth), static_cast<int>(this->fHeight));

    QColor color = this->palette().background().color();

    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(this->voxelData == nullptr)
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    QVector3D extends = this->voxelData->filteredData()->getDimensionsMetric();
    QVector3D origin = this->voxelData->filteredData()->getFirstVoxelPosition();
    float scaling = extends.x();

    ///////////////////////////////////////////////////////////////////////////////////
    /// Setup projection and view matrix
    ///////////////////////////////////////////////////////////////////////////////////
    glMatrixMode(GL_PROJECTION);
    {
        QMatrix4x4 matViewProj = view3d->projectionMatrix(scaling, this->fWidth, this->fHeight) * view3d->viewMatrix(scaling);

        glLoadMatrixf(matViewProj.constData());
    }
    glMatrixMode(0);

    ///////////////////////////////////////////////////////////////////////////////////
    /// Render data set cuboid
    ///////////////////////////////////////////////////////////////////////////////////
    glMatrixMode(GL_MODELVIEW);
    {
        glLoadIdentity();
        glTranslatef(origin.x(), origin.y(), origin.z());
    }
    glMatrixMode(0);

    if(this->lists.size() > 0)
    {
        glCallLists(this->lists.size(), GL_UNSIGNED_INT, this->lists.data());
    }

    this->update();
}


void IsosurfaceView::mousePressEvent(QMouseEvent *event)
{
    view3d->mousePressEvent(mouseLast, event);
	this->mouseLast = event->pos();
}

void IsosurfaceView::mouseMoveEvent(QMouseEvent *event)
{
    view3d->mouseMoveEvent(mouseLast, event);
	this->mouseLast = event->pos();
}

void IsosurfaceView::wheelEvent(QWheelEvent *event)
{
    view3d->wheelEvent(event);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
