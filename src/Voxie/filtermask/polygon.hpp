#pragma once

#include <Voxie/filtermask/polygonData.hpp>
#include <Voxie/filtermask/shapes.hpp>

namespace voxie{
namespace filter{
/**
 * @brief The polygon class offers methods for creating a polygon shape.
 */
class polygon : public shapes
{
public:

	polygon(QVector<QPointF>polygonCoords);

	virtual void translateOrigin(qreal x, qreal y) override;
	virtual void rotate(qreal angle) override;

    /**
     * @brief getTransformatedPolyCoords
     * @return return the transformated coordinates of the polygon.
     */
	polygonData getTransformatedPolyCoords();

    /**
     * @brief path creates a QPainterPath from the shape.
     * @return the shape as a QPainterPath Object
     */
	QPainterPath path();

    /**
     * @brief pointFVectorToPoly converts a QVector full of QPointF's to a Vector with polygonPoint.
     * @return a QVector with polygonPoint.
     */
	QVector<polygonPoint> pointFVectorToPoly(QVector<QPointF>);

private:

	QVector<QPointF> polygonCoords;
	polygonData data;

	QMatrix transformationMatrix;

    QMatrix transformationRotate;
    QMatrix transformationScale;
    QMatrix transformationTranslate;
};
}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
