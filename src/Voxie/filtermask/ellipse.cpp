#include "ellipse.hpp"

#include <QtCore/QDebug>

#include <QtGui/QPainterPath>

using namespace voxie::filter;


/*
 * m11 m12 0
 * m21 m22 0
 * dx  dy  1
 *
 * m11 = horizontale Skalierung
 * m22 = vertikale Skalierung
 *
 * m21 = horizontale drehung
 * m12 = vertikale drehung
 *
 * dx = translation x
 * dy = translation y
 */
ellipse::ellipse(qreal midPointX, qreal midPointY, qreal radiusX, qreal radiusY)
{
	//Constructor vertikale Skalierung, vertikale Drehung, horizontale Drehung, horizontale Skalierung, translate X, translate Y

    this->transformationRotate = QTransform(1, 0, 0, 1, 0, 0);
    this->transformationScale = QTransform(radiusX, 0, 0, radiusY, 0, 0);

    this->transformationTranslate = QTransform(1, 0, 0, 1, midPointX, midPointY);
    this->transformationMatrix = this->transformationScale * this->transformationRotate * this->transformationTranslate;

	if(!this->transformationMatrix.isInvertible())
	{
        //qDebug() << "Notinvertible";
	}
    QTransform inverted = this->transformationMatrix.inverted();
    data.scaleX = (cl_float) inverted.m11();
    data.scaleY = (cl_float) inverted.m22();
    data.angleX = (cl_float) inverted.m21();
    data.angleY = (cl_float) inverted.m12();
    data.dx = (cl_float) inverted.dx();
    data.dy = (cl_float) inverted.dy();

}

QPainterPath ellipse::path()
{
	QPainterPath path;
	path.addEllipse(QPointF(0, 0), 1, 1);
	path = this->transformationMatrix.map(path);
	return path;
}

void ellipse::translateOrigin(qreal x, qreal y)
{

    this->transformationTranslate = this->transformationTranslate.translate(x, y);
    this->transformationMatrix = this->transformationScale* this->transformationRotate * this->transformationTranslate;


    this->transformationMatrix = transformationScale * transformationRotate * transformationTranslate;

	if(!this->transformationMatrix.isInvertible())
	{
        //qDebug() << "Notinvertible";
	}
    QTransform inverted = this->transformationMatrix.inverted();
    data.scaleX = (cl_float) inverted.m11();
    data.scaleY = (cl_float) inverted.m22();
    data.angleX = (cl_float) inverted.m21();
    data.angleY = (cl_float) inverted.m12();
    data.dx = (cl_float) inverted.dx();
    data.dy = (cl_float) inverted.dy();

}

void ellipse::rotate(qreal angle)
{

    this->transformationRotate = this->transformationRotate.rotate(angle);

    QPointF translatePoint(this->transformationTranslate.dx(),this->transformationTranslate.dy());

    QTransform tempRotate(1, 0, 0, 1, 0, 0);
    tempRotate.rotate(angle);
    QPointF rotatedPoint = tempRotate.map(translatePoint);
    this->transformationTranslate = QTransform(1, 0, 0, 1, rotatedPoint.x(), rotatedPoint.y());

    this->transformationMatrix = this->transformationScale* this->transformationRotate * this->transformationTranslate;

	if(!this->transformationMatrix.isInvertible())
	{
        //qDebug() << "Notinvertible";
	}
    QTransform inverted = this->transformationMatrix.inverted();
    data.scaleX = (cl_float) inverted.m11();
    data.scaleY = (cl_float) inverted.m22();
    data.angleX = (cl_float) inverted.m21();
    data.angleY = (cl_float) inverted.m12();
    data.dx = (cl_float) inverted.dx();
    data.dy = (cl_float) inverted.dy();
}

ellipseData ellipse::getTransformationMatrix()
{
	return data;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
