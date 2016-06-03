#pragma once

#include <Voxie/filtermask/rectangleData.hpp>
#include <Voxie/filtermask/shapes.hpp>

#include <QtGui/QTransform>

using namespace voxie::filter;
/**
 * @brief The rectangle class offers methods for creating a rectangle shape for the masks.
 */
class rectangle : public shapes
{
public:
	rectangle(qreal startX, qreal startY, qreal endX, qreal endY);
	virtual void rotate(qreal angle) override;
	virtual void translateOrigin(qreal x, qreal y) override;
    /**
     * @brief path creates a QPainterPath from the shape.
     * @return the shape as a QPainterPath Object
     */
	QPainterPath path();

    /**
     * @brief getTransformationMatrix
     * @return the inverted Transformation matrix.
     */
	rectangleData getTransformationMatrix();
	void isPointIn(QPointF point);
private:
	QPointF leftUp = QPointF(0,1);
	QPointF leftDown = QPointF(0, 0);
	QPointF rightUp = QPointF(1,1);
	QPointF rightDown = QPointF(1, 0);

	rectangleData data;

    QTransform transformationMatrix;
    QTransform transformationRotate;
    QTransform transformationScale;
    QTransform transformationTranslate;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
