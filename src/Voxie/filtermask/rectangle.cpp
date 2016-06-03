#include "rectangle.hpp"

#include <math.h>

#include <QtCore/QDebug>

using namespace voxie::filter;

//constructor

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
rectangle::rectangle(qreal startX, qreal startY, qreal endX, qreal endY)
{

	//Set startPoints.

	//Constructor vertikale Skalierung, vertikale Drehung, horizontale Drehung, horizontale Skalierung, translate X, translate Y

    this->transformationRotate = QTransform(1, 0, 0, 1, 0, 0);
    this->transformationScale = QTransform(fabs(endX - startX), 0, 0, fabs(endY - startY), 0, 0);
    //falls startpunkt unten links endpunkt oben rechts
    if (startX < endX && startY > endY)
    {
        //startX = endX - fabs(endX - startX);
        startY = endY;
    }
    //falls startpunkt oben rechts und endpunkt unten links
    else if(startX > endX && startY < endY)
    {
        startX = endX;
        //startY = startY - fabs(startY - endY);
    }
    //falls startpunkt unten rechts und endpunkt oben links
    else if(startX > endX && startY > endY)
    {
        int temp = startX;
        startX = endX;
        endX = temp;
        temp  = startY;
        startY = endY;
        endY = temp;
    }
    this->transformationTranslate = QTransform(1, 0, 0, 1, startX, startY);
    this->transformationMatrix = this->transformationScale * this->transformationRotate;
    this->transformationMatrix *= this->transformationTranslate;

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

void rectangle::translateOrigin(qreal x, qreal y)
{
	//translate the coordinates in rectangle struct

	//test translation

    this->transformationTranslate = this->transformationTranslate.translate(x, y);

    this->transformationMatrix = this->transformationScale * this->transformationRotate * this->transformationTranslate;

	if(!this->transformationMatrix.isInvertible())
	{
        //qDebug() << "Not invertible";
	}
    QTransform inverted = this->transformationMatrix.inverted();
    data.scaleX = (cl_float) inverted.m11();
    data.scaleY = (cl_float) inverted.m22();
    data.angleX = (cl_float) inverted.m21();
    data.angleY = (cl_float) inverted.m12();
    data.dx = (cl_float) inverted.dx();
    data.dy = (cl_float) inverted.dy();

}

void rectangle::rotate(qreal angle)
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
        //qDebug() << "Not invertible";
	}
    QTransform inverted = this->transformationMatrix.inverted();

    data.scaleX = (cl_float) inverted.m11();
    data.scaleY = (cl_float) inverted.m22();
    data.angleX = (cl_float) inverted.m21();
    data.angleY = (cl_float) inverted.m12();
    data.dx = (cl_float) inverted.dx();
    data.dy = (cl_float) inverted.dy();
}

QPainterPath rectangle::path()
{
	QPainterPath path;
    path.addRect(QRectF(0,0, 1, 1));
	path = this->transformationMatrix.map(path);
	return path;
}

void rectangle::isPointIn(QPointF point)
{

    Q_UNUSED(point);
/*	QPointF resultLeftUp = this->transformationMatrix.map(this->leftUp);
	QPointF resultLeftDown = this->transformationMatrix.map(this->leftDown);
	QPointF resultRightUp = this->transformationMatrix.map(this->rightUp);
	QPointF resultRightDown = this->transformationMatrix.map(this->rightDown);

    QTransform inverted = this->transformationMatrix.inverted();
	QPointF result = inverted.map(point);
  //  qDebug() << this->transformationMatrix.isInvertible();*/

}

rectangleData rectangle::getTransformationMatrix()
{
	return this->data;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
