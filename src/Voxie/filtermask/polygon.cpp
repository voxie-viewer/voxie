#include "polygon.hpp"

#include <QtCore/QDebug>

using namespace voxie::filter;

polygon::polygon(QVector<QPointF> polygonCoords)
{
	this->polygonCoords = polygonCoords;
    data.cpuCoords = polygonCoords;
    data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
    this->transformationRotate = QMatrix(1, 0, 0, 1, 0, 0);
    this->transformationScale = QMatrix(1, 0, 0, 1, 0, 0);
    this->transformationTranslate = QMatrix(1, 0, 0, 1, 0, 0);
    this->transformationMatrix = this->transformationScale * this->transformationRotate * this->transformationTranslate;

}

void polygon::translateOrigin(qreal x, qreal y)
{
	data.cpuCoords.clear();
	data.gpuCoords.clear();

    this->transformationTranslate = this->transformationTranslate.translate(x, y);

    this->transformationMatrix = this->transformationScale * this->transformationRotate * this->transformationTranslate;

	for (int i = 0; i < polygonCoords.size(); i++)
	{
		QPointF result = this->transformationMatrix.map(polygonCoords.at(i));
		data.cpuCoords.append(result);
	}
	data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
}

void polygon::rotate(qreal angle)
{
	data.cpuCoords.clear();
	data.gpuCoords.clear();


    this->transformationRotate = this->transformationRotate.rotate(angle);

    QPointF translatePoint(this->transformationTranslate.dx(),this->transformationTranslate.dy());

    QMatrix tempRotate(1, 0, 0, 1, 0, 0);
    tempRotate.rotate(angle);
    QPointF rotatedPoint = tempRotate.map(translatePoint);
    this->transformationTranslate.setMatrix(1, 0, 0, 1, rotatedPoint.x(), rotatedPoint.y());

    //qDebug() << this->transformationTranslate;
    this->transformationMatrix = this->transformationScale* this->transformationRotate * this->transformationTranslate;

	for (int i = 0; i < polygonCoords.size(); i++)
	{
		QPointF result = this->transformationMatrix.map(polygonCoords.at(i));
        data.cpuCoords.append(result);
	}

	data.gpuCoords = this->pointFVectorToPoly(data.cpuCoords);
}

QPainterPath polygon::path()
{
	QPainterPath path;
	path.addPolygon(QPolygonF(this->polygonCoords));
	path = this->transformationMatrix.map(path);
	return path;
}

polygonData polygon::getTransformatedPolyCoords()
{
	return data;
}
//from cpuCoords to gpuCoords
QVector<polygonPoint> polygon::pointFVectorToPoly(QVector<QPointF> cpu)
{
	QVector<polygonPoint> point;
	for(int i = 0; i < cpu.size(); i++)
	{
		polygonPoint addPoint;
        addPoint.x = (cl_float) cpu.at(i).x();
        addPoint.y = (cl_float) cpu.at(i).y();
		point.append(addPoint);
	}
	return point;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
