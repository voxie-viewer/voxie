#include "selection2dmask.hpp"

#include <QtCore/QDebug>

using namespace voxie::filter;
using namespace voxie::opencl;


//----------------DELETE-MASKS------------------------------------
void Selection2DMask::clearMask()
{
    mutex.lock();
    if (ellipses.size() != 0)
    {
        this->ellipses.clear();
    }
    if (rectangles.size() != 0)
    {
        this->rectangles.clear();
    }
    if(polygons.size() != 0)
    {
        this->polygons.clear();
    }
    mutex.unlock();
    emit this->changed();
}


//---------------ADD-MASKS-----------------------------------------
void Selection2DMask::addEllipse(qreal midPointX, qreal midPointY, qreal radiusX, qreal radiusY)
{
    mutex.lock();
	this->ellipses.append(new ellipse(midPointX, midPointY, radiusX, radiusY));
    mutex.unlock();
    emit this->changed();
}

void Selection2DMask::addRectangle(qreal startX, qreal startY, qreal endX, qreal endY)
{
    mutex.lock();
	this->rectangles.append(new rectangle(startX, startY, endX, endY));
    mutex.unlock();
    emit this->changed();
}

void Selection2DMask::addPolygon(QVector<QPointF> polygonCoords)
{
    mutex.lock();
	this->polygons.append(new polygon(polygonCoords));
    mutex.unlock();
    emit this->changed();
}

//----------------CPU-MASKS------------------------------------------

QVector<rectangleData> Selection2DMask::getRectangleCoords()
{
	QVector<rectangleData> returnData;
    mutex.lock();
	for(int i = 0; i < this->rectangles.size(); i++)
	{
		returnData.append(this->rectangles.at(i)->getTransformationMatrix());
	}
    mutex.unlock();
	return returnData;
}


QVector<ellipseData> Selection2DMask::getEllipseCoords()
{
	QVector<ellipseData> returnData;
    mutex.lock();
	for (int i = 0; i < this->ellipses.size();i++)
	{
		returnData.append(this->ellipses.at(i)->getTransformationMatrix());
	}
    mutex.unlock();
	return returnData;
}


QVector<polygonData> Selection2DMask::getPolygonCoords()
{
	QVector<polygonData> returnData;
    mutex.lock();
	for (int i = 0; i < this->polygons.size();i++)
	{
		returnData.append(this->polygons.at(i)->getTransformatedPolyCoords());
	}
    mutex.unlock();
	return returnData;
}

//-----------------------POINT-CHECK-CPU-----------------------------------

bool Selection2DMask::isPointIn(QPointF coords)
{
    bool result = false;
	/*
	 *  Checks every single shape, if one shape contains the point the result turns to true.
	 *
	 */

	//checks if a rectangle shape contains the coordinate
    QRectF identityRect = QRectF(0,0, 1, 1);
    auto rectangleCoords = this->getRectangleCoords();
    for (int i = 0; (i < rectangleCoords.size() && !result); i++)
	{
        rectangleData rect = rectangleCoords.at(i);
        QTransform matrixRect(rect.scaleX, rect.angleY, rect.angleX, rect.scaleY, rect.dx, rect.dy);
        QPointF translatedCoords = matrixRect.map(coords);
		QPainterPath check;
        check.addRect(identityRect);
        if (check.contains(translatedCoords))
		{
			result = true;
		}
	}

    //checks if a ellipse shape contains the coordinate
    auto ellipseCoords = this->getEllipseCoords();
    for (int i = 0; (i < ellipseCoords.size() && !result); i++)
	{
        ellipseData ellipse = ellipseCoords.at(i);
        QTransform matrixEllipse(ellipse.scaleX, ellipse.angleY, ellipse.angleX, ellipse.scaleY, ellipse.dx, ellipse.dy);
		QPointF translatedCoords = matrixEllipse.map(coords);
		QPainterPath check;
        check.addEllipse(QPointF(0, 0),1,1);
		if (check.contains(translatedCoords))
        {
			result = true;
		}

	}

	//checks if a polygon shape contains the coordinate
    auto polygonCoords = this->getPolygonCoords();
    for (int i = 0; (i < polygonCoords.size() && !result); i++)
	{
        polygonData polygon = polygonCoords.at(i);
		QPolygonF createdPoly(polygon.cpuCoords);
		QPainterPath check;
		check.addPolygon(createdPoly);
		if(check.contains(coords))
		{

			result = true;
		}
    }

	return result;
}

//-------------------GPU-MASKS--------------------------------------

    cl::Buffer Selection2DMask::getRectangleBuffer(CLInstance* instance)
	{
        auto rectangleCoords = this->getRectangleCoords();
        if(rectangleCoords.isEmpty()){
            // buffer with one element since no elements are not allowed
            return instance->createBuffer(sizeof(rectangleData));
        }

        size_t size = (sizeof(rectangleData) * rectangleCoords.size());
        cl::Buffer buffer = instance->createBuffer(size, rectangleCoords.data());
		return buffer;
	}

    cl::Buffer Selection2DMask::getEllipseBuffer(CLInstance* instance)
	{
        auto ellipseCoords = this->getEllipseCoords();
        if(ellipseCoords.isEmpty()){
            // buffer with one element since no elements are not allowed
            return instance->createBuffer(sizeof(ellipseData));
        }

        size_t size = (sizeof(ellipseData) * ellipseCoords.size());
        cl::Buffer buffer = instance->createBuffer(size, ellipseCoords.data());
		return buffer;
	}

    cl::Buffer Selection2DMask::getPolygonBufferOffset(CLInstance* instance)
	{
        auto polygonCoords = this->getPolygonCoords();
        if(polygonCoords.isEmpty()){
            // buffer with one element since no elements are not allowed
            return instance->createBuffer(sizeof(cl_int));
        }
		cl::Buffer offsetBuffer;
		QVector<cl_int> offset;
		offset.append(0);
        for (int i = 0; i < polygonCoords.size(); i++)
		{
            offset.append(polygonCoords.at(i).gpuCoords.size() + offset.at(i));
		}
		size_t size = sizeof(cl_int) * offset.size();
        offsetBuffer = instance->createBuffer(size, offset.data());
		return offsetBuffer;
	}

    cl::Buffer Selection2DMask::getPolygonBuffer(CLInstance* instance)
	{
        auto polygonCoords = this->getPolygonCoords();
        if(polygonCoords.isEmpty()){
            // buffer with one element since no elements are not allowed
            return instance->createBuffer(sizeof(polygonPoint));
        }
		QVector<polygonPoint> coords;
        for (int i = 0; i < polygonCoords.size(); i++)
		{
            for (int j = 0; j < polygonCoords.at(i).gpuCoords.size(); j++)
			{
                coords.append(polygonCoords.at(i).gpuCoords.at(j));
			}
		}
		size_t size = sizeof(polygonPoint) * coords.size();
        cl::Buffer polygons = instance->createBuffer(size, coords.data());
		return polygons;
	}

void Selection2DMask::translateOrigin(qreal x, qreal y)
{
   this->mutex.lock();
   for (int i = 0; i < rectangles.size(); i++)
   {
       rectangles.at(i)->translateOrigin(x, y);
   }

   for (int i = 0; i < ellipses.size(); i++)
   {
       ellipses.at(i)->translateOrigin(x, y);
   }

   for (int i = 0; i < polygons.size(); i++)
   {
       polygons.at(i)->translateOrigin(x, y);
   }
   this->mutex.unlock();
}

void Selection2DMask::rotate(qreal angel)
{
    this->mutex.lock();
    for (int i = 0; i < rectangles.size(); i++)
    {
        rectangles.at(i)->rotate(angel);
    }

    for (int i = 0; i < ellipses.size(); i++)
    {
        ellipses.at(i)->rotate(angel);
    }

    for (int i = 0; i < polygons.size(); i++)
    {
        polygons.at(i)->rotate(angel);
    }
    this->mutex.unlock();
}

//----------------PAINTER-PATH----------------------------------
QPainterPath Selection2DMask::getPath()
{
    QPainterPath result;
    mutex.lock();
    for (int i = 0; i < rectangles.size(); i++)
    {
        result = result.united(rectangles.at(i)->path());
    }

    for (int i = 0; i < ellipses.size(); i++)
    {
        result = result.united(ellipses.at(i)->path());
    }

    for (int i = 0; i < polygons.size(); i++)
    {
        result = result.united(polygons.at(i)->path());
    }
    mutex.unlock();
    result.simplified();
    return result;

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
