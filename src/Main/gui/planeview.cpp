#include "planeview.hpp"

#include <cmath>

#include <QtGui/QMatrix4x4>
#include <QtGui/QMouseEvent>

#include <QtOpenGL/QGLFormat>

using namespace voxie::gui;
using namespace voxie::data;

PlaneView::PlaneView(Slice *slice, QWidget *parent) :
	QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::Rgba), parent),
	slice(slice),
	fWidth(1), fHeight(1),
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

void PlaneView::initializeGL()
{

}

void PlaneView::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);

	this->fWidth = static_cast<float>(w);
	this->fHeight = static_cast<float>(h);

	this->fHeight = std::max<float>(this->fHeight, 1);
}

void PlaneView::paintGL()
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClearDepth(1.0f);

	if(this->slice == nullptr)
	{
		return;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	QVector3D extends = this->slice->getDataset()->size();
	QVector3D origin = this->slice->getDataset()->origin();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	///////////////////////////////////////////////////////////////////////////////////
	/// Setup projection and view matrix
	///////////////////////////////////////////////////////////////////////////////////
	glMatrixMode(GL_PROJECTION);
	{
        QMatrix4x4 matViewProj = view3d->projectionMatrix(this->fWidth, this->fHeight) * view3d->viewMatrix();

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
	glBegin(GL_QUADS);

		// Front
		glColor3f(0.8f, 0.8f, 0.8f);
		glVertex3f(0,		   0,		   extends.z());
		glVertex3f(extends.x(), 0,		   extends.z());
		glVertex3f(extends.x(), extends.y(), extends.z());
		glVertex3f(0,		   extends.y(), extends.z());

		// Back
		glColor3f(0.4f, 0.4f, 0.4f);
		glVertex3f(0,		   0,		   0);
		glVertex3f(extends.x(), 0,		   0);
		glVertex3f(extends.x(), extends.y(), 0);
		glVertex3f(0,		   extends.y(), 0);

		// Left
		glColor3f(0.6f, 0.6f, 0.6f);
		glVertex3f(extends.x(), 0,		   0);
		glVertex3f(extends.x(), 0,		   extends.z());
		glVertex3f(extends.x(), extends.y(), extends.z());
		glVertex3f(extends.x(), extends.y(), 0);

		// Right
		glColor3f(0.5f, 0.5f, 0.5f);
		glVertex3f(0, 0,		   0);
		glVertex3f(0, 0,		   extends.z());
		glVertex3f(0, extends.y(), extends.z());
		glVertex3f(0, extends.y(), 0);

		// Top
		glColor3f(0.7f, 0.7f, 0.7f);
		glVertex3f(0,		   extends.y(), 0);
		glVertex3f(0,		   extends.y(), extends.z());
		glVertex3f(extends.x(), extends.y(), extends.z());
		glVertex3f(extends.x(), extends.y(), 0);

		// Bottom
		glColor3f(0.3f, 0.3f, 0.3f);
		glVertex3f(0,		   0,  0);
		glVertex3f(0,		   0, extends.z());
		glVertex3f(extends.x(), 0, extends.z());
		glVertex3f(extends.x(), 0, 0);

	glEnd();

	///////////////////////////////////////////////////////////////////////////////////
	/// Render coordinate axis
	///////////////////////////////////////////////////////////////////////////////////
	glMatrixMode(GL_MODELVIEW);
	{
		glLoadIdentity();
	}
	glMatrixMode(0);
	glBegin(GL_LINES);
    {
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(2.0f * extends.x(), 0.0f, 0.0f);

		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 2.0f * extends.y(), 0.0f);

		glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 2.0f * extends.z());
    }
	glEnd();

	///////////////////////////////////////////////////////////////////////////////////
	/// Render plane
	///////////////////////////////////////////////////////////////////////////////////
	glMatrixMode(GL_MODELVIEW);
	{
		QMatrix4x4 transform;


		// Adjust origin
		//transform.translate(origin);

		// Slice transformation
		transform.translate(this->slice->origin());
		transform.rotate(this->slice->rotation());

		glLoadIdentity();
		glLoadMatrixf(transform.constData());
	}
    glMatrixMode(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);

		glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
        glVertex3f(-extends.length(), -extends.length(), 0);
        glVertex3f(-extends.length(), extends.length(),  0);
        glVertex3f( extends.length(), extends.length(),  0);
        glVertex3f( extends.length(), -extends.length(), 0);

	glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBegin(GL_LINES);
    {
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(extends.length(), 0.0f, 0.0f);

        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, extends.length(), 0.0f);

        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, extends.length());
    }
    glEnd();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
