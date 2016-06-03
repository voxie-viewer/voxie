#pragma once

#include <Voxie/filtermask/ellipseData.hpp>
#include <Voxie/filtermask/shapes.hpp>

#include <QtGui/QTransform>

namespace voxie{

namespace filter{
/**
 * @brief The ellipse class offers methods for creatin a ellipse shape.
 */
class ellipse : public shapes
{
public:
	ellipse(qreal midPointX, qreal midPointY, qreal radiusX, qreal radiusY);
	virtual void rotate(qreal angle) override;
	virtual void translateOrigin(qreal x, qreal y) override;
    /**
     * @brief path creates a QPainterPath from the shape.
     * @return the shape as a QPainterPath Object
     */
	QPainterPath path();

    /**
     * @brief getTransformationMatrix
     * @return the inverted transformation Matrix.
     */
	ellipseData getTransformationMatrix();

private:

	ellipseData data;

    QTransform transformationTranslate;
    QTransform transformationRotate;
    QTransform transformationScale;
    QTransform transformationMatrix;


};
}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
