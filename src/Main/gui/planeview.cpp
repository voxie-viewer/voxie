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
    pan(0.0f), tilt(0.0f), zoom(1.5f)
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
}

void PlaneView::mousePressEvent(QMouseEvent *event)
{
	this->mouseLast = event->pos();
}

void PlaneView::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - this->mouseLast.x();
	int dy = event->y() - this->mouseLast.y();

    if (event->buttons() & Qt::LeftButton)
    {
		this->pan += 0.02f * dx;
		this->tilt += 0.02f * dy;
		if(this->tilt > 0.49f * M_PI)
			this->tilt = (float)(0.49f * M_PI);
		if(this->tilt < -0.49f * M_PI)
			this->tilt = (float)(-0.49f * M_PI);
		this->repaint();
	}
    else if((event->buttons() & Qt::RightButton) && (this->slice != nullptr))
    {
        float ax = 0.12f * dx;
        float ay = 0.12f * dy;

        QQuaternion src = this->slice->rotation();

        QMatrix4x4 matViewProj;
        QVector3D position(
                cosf(this->pan) * cosf(this->tilt),
                sinf(this->tilt),
                sinf(this->pan) * cosf(this->tilt));
        matViewProj.lookAt(
                    position,
                    QVector3D(0.0f, 0.0f, 0.0f),
                    QVector3D(0.0f, 1.0f, 0.0f));
        matViewProj.setRow(3, QVector4D(0,0,0,1)); // Remove translation

        QQuaternion quatX = QQuaternion::fromAxisAndAngle((QVector4D(0, 1, 0, 0) * matViewProj).toVector3D(), ax);
        QQuaternion quatY = QQuaternion::fromAxisAndAngle((QVector4D(1, 0, 0, 0) * matViewProj).toVector3D(), ay);

        QQuaternion quat = quatX * quatY;

        this->slice->setRotation(quat * src);

        this->repaint();
    }
	this->mouseLast = event->pos();
}

void PlaneView::wheelEvent(QWheelEvent *event)
{
	this->zoom += 0.1f * event->angleDelta().y() / 120.0f;
	if(this->zoom < 1.5f)
		this->zoom = 1.5f;
	if(this->zoom > 2.5f)
		this->zoom = 2.5f;
	this->repaint();
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
	float scaling = extends.x();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	///////////////////////////////////////////////////////////////////////////////////
	/// Setup projection and view matrix
	///////////////////////////////////////////////////////////////////////////////////
	glMatrixMode(GL_PROJECTION);
	{
		QVector3D position(
				cosf(this->pan) * cosf(this->tilt),
				sinf(this->tilt),
				sinf(this->pan) * cosf(this->tilt));
        position *= this->zoom * scaling;

		QMatrix4x4 matViewProj;
		matViewProj.perspective(
					80,
					this->fWidth / this->fHeight,
					0.1f * scaling,
					10000.0f * scaling);
		matViewProj.lookAt(
					position,
					QVector3D(0.0f, 0.0f, 0.0f),
					QVector3D(0.0f, 1.0f, 0.0f));

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
