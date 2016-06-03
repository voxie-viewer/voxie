#pragma once

#include <Voxie/scripting/dbustypes.hpp>

#include <cmath>

#include <QtCore/QPointF>
#include <QtCore/QVector>

#include <QtGui/QMatrix4x4>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>

namespace voxie {
namespace data {


inline QVector3D invalidVec3()
{double n = nan(""); return QVector3D(n,n,n);}

inline bool isInvalidVec3(const QVector3D& vec)
{
    return std::isnan(QVector3D::dotProduct(vec,vec));
}

/**
 * @brief The Plane struct defines a plane in 3D space.
 */
struct Plane
{
Plane () {}
Plane (QVector3D origin, QQuaternion rotation) : origin(origin), rotation(rotation) {}
Plane (const voxie::scripting::Plane& plane) : origin(plane.origin), rotation(plane.rotation) {}
operator voxie::scripting::Plane () const {
    voxie::scripting::Plane plane;
    plane.origin = origin;
    plane.rotation = rotation;
    return plane;
}

/**
 * @brief origin of the planes coordinatesystem
 */
QVector3D origin;
/**
 * @brief rotation of the plane
 */
QQuaternion rotation;

/**
 * @brief normal vector of this plane (perpendicular to plane).
 * @return  normal vector (unit vector)
 */
QVector3D normal() const
{
    return rotation.rotatedVector(QVector3D(0,0,-1));
}

/**
 * @brief tangent vector, equivalent to planes x-axis
 * @return tangent vector (unit vector)
 */
QVector3D tangent() const
{
    return rotation.rotatedVector(QVector3D(1,0,0));
}

/**
 * @brief cotangent vector, equivalent to planes y-axis
 * @return cotangent vector (unit vector)
 */
QVector3D cotangent() const
{
    return rotation.rotatedVector(QVector3D(0,1,0));
}

/**
 * @brief get3DPoint translates a 2D point on the coordinatesystem spanned by the plane
 * to the corresponding point in 3D space.
 * @param x
 * @param y
 * @return point in 3D
 */
QVector3D get3DPoint(qreal x, qreal y) const
{
    QVector3D p = origin + (tangent()*x) + (cotangent()*y);
    return  p - (distance(p)*normal()); // fix possible offset due to inaccuracy
}

/**
 * @see get3DPoint(qreal x, qreal y)
 */
QVector3D get3DPoint(const QPointF& point) const
{
    return get3DPoint(point.x(), point.y());
}

/**
 * @see get3DPoint(qreal x, qreal y)
 * @param destination
 */
void get3DPoint(const QPointF &point, QVector3D& destination) const
{
    destination = origin + (tangent()*point.x()) + (cotangent()*point.y());
}

/**
 * @brief projects vec onto plane and returns 2D point on plane
 * @param vec 3D point to be projected
 * @return 2D plane point of projected vector
 */
QPointF get2DPlanePoint(const QVector3D& vec) const
{
    QVector3D intersection = intersectLine(vec, normal());

    QMatrix4x4 translation; translation.setToIdentity();
    translation.translate(origin);
    QMatrix4x4 rotationM; rotationM.setToIdentity();
    rotationM.setColumn(0, tangent().toVector4D());
    rotationM.setColumn(1, cotangent().toVector4D());
    rotationM.setColumn(2, normal().toVector4D());
    QMatrix4x4 transform = translation * rotationM;
    transform = transform.inverted();

    return (transform * intersection).toPointF();
}

/**
 * @brief intersectLine calculates the intersection point of the plane with given line.
 * @param linePoint
 * @param lineDirection
 * @return intersection point or invalidVec3 if line is parallel
 */
QVector3D intersectLine(const QVector3D& linePoint, const QVector3D& lineDirection) const
{
    float denominator = QVector3D::dotProduct( lineDirection, normal() );
    if( fabs(denominator) < 1e-6 /* isZero */){
        // parallel
        return invalidVec3();
    } else {
        float numerator = QVector3D::dotProduct( (origin - linePoint), normal() );
        return linePoint + ((numerator/denominator) * lineDirection);
    }
    return linePoint;
}

/**
 * @brief intersectVolume calculates the bounding polygon of the planes intersection with given volume.
 * @param volumeOrigin
 * @param volumeDimension
 * @return bounding polygon of intersection (unordered points)
 */
QVector<QVector3D> intersectVolume(const QVector3D& volumeOrigin, const QVector3D& volumeDimension) const
{
    // corners of volume box
    QVector3D bottomCorners[4] = {  QVector3D(0,0,0) -volumeOrigin,
                                    QVector3D(0,0,volumeDimension.z()) -volumeOrigin,
                                    QVector3D(0,volumeDimension.y(),0) -volumeOrigin,
                                    QVector3D(0,volumeDimension.y(),volumeDimension.z()) -volumeOrigin};
    QVector3D topCorners[4];
    for(size_t i =0; i < 4; i++){
        topCorners[i] = bottomCorners[i] + QVector3D(volumeDimension.x(),0,0);
    }

    QVector<QVector3D> intersectionPoints;
    QVector3D vec;
    // check intersections with volume box
    {
    // check bottom lines
    vec = intersectLine(bottomCorners[0], QVector3D(0,0,1));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(bottomCorners[0], QVector3D(0,1,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(bottomCorners[3], QVector3D(0,0,1));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(bottomCorners[3], QVector3D(0,1,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    // check top lines
    vec = intersectLine(topCorners[0], QVector3D(0,0,1));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(topCorners[0], QVector3D(0,1,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(topCorners[3], QVector3D(0,0,1));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(topCorners[3], QVector3D(0,1,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    //check side lines
    vec = intersectLine(bottomCorners[0], QVector3D(1,0,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(bottomCorners[1], QVector3D(1,0,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(bottomCorners[2], QVector3D(1,0,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}

    vec = intersectLine(bottomCorners[3], QVector3D(1,0,0));
    if(!isInvalidVec3(vec)){intersectionPoints.append(vec);}
    }

    // remove out of box intersections
    for(int i = 0; i < intersectionPoints.length();){
        vec = intersectionPoints[i] + volumeOrigin;
        if(vec.x() < 0 || vec.y() < 0 || vec.z() < 0 || vec.x() > volumeDimension.x() ||
                vec.y() > volumeDimension.y() || vec.z() > volumeDimension.z() )
        {
            intersectionPoints.removeAt(i);
        } else {
            i++;
        }
    }

    return intersectionPoints;
}

/**
 * @brief isOnPlane checks if a point is on the plane.
 * @param point
 * @return true if distance to point is zero.
 */
bool isOnPlane(const QVector3D& point) const
{
    return ( fabs(distance(point)) < 1e-6);
}

/**
 * @return distance of point to the plane, may be negative if is below the plane.
 */
float distance(const QVector3D& point) const
{
    return point.distanceToPlane( origin, normal() );
}

/**
 * @return distance as in hesse normal form of a plane (non-negative)
 */
float distance() const
{
    return (float) fabs( distance(QVector3D(0,0,0)) );
}

};

}}
Q_DECLARE_METATYPE(voxie::data::Plane)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
