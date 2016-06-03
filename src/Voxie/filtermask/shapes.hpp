#pragma once

#include <QtGui/QPainterPath>

namespace voxie{
namespace filter{
/**
 * @brief The shapes class is a interface of the shapes rectangle, polygon and ellipse.
 */
class shapes
{
    /**
     * @brief translateOrigin translates the origin of the masks.
     * @param x amount of translation in X Direction.
     * @param y amount of translation in Y Direction.
     */
    virtual void translateOrigin(qreal x, qreal y)=0;

    /**
     * @brief rotate the matrix
     * @param angle the angle for the rotation.
     */
    virtual void rotate(qreal angle)=0;
};
}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
