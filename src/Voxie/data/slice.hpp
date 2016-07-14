#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/interpolationmethod.hpp>
#include <Voxie/data/plane.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtCore/QObject>
#include <QtCore/QRectF>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>

namespace voxie {
namespace data {

/**
 * @brief The Slice class is a Plane associated to a DataSet instance.
 * The plane is placed inside the DataSet's coordinatesystem.
 * It is used to extract SliceImages from the voxel dataset.
 */
class VOXIECORESHARED_EXPORT Slice : public voxie::scripting::ScriptableObject
{
    Q_OBJECT
public:
    explicit Slice(DataSet* parent);

    Q_PROPERTY (voxie::data::Plane Plane READ getCuttingPlane WRITE setPlane)
    /**
     * @return this Slice's Plane (a copy of it).
     */
    // return type needs to be fully qualified for moc
    voxie::data::Plane getCuttingPlane() const
        {return this->cuttingPlane;}
    void setPlane(const voxie::data::Plane& plane);


    Q_PROPERTY (voxie::data::DataSet* dataSet READ getDataset)
    /**
     * @return this Slice's DataSet instance.
     */
    // return type needs to be fully qualified for moc
    voxie::data::DataSet* getDataset() const
        {return this->dataset;}

    /**
     * @param point on slice plane.
     * @param interpolation method for obtaining the value
     * @return value of the datasets voxel that is located at given point on the plane.
     */
    float getPixelValue(QPointF point, InterpolationMethod interpolation = linear) const
        {return getPixelValue(point.x(), point.y(), interpolation);}

    /**
     * @param x coordinate of point on slice plane
     * @param y coordinate of point on slice plane
     * @param interpolation method for obtaining the value
     * @return value of the datasets voxel that is located at given point on the plane.
     */
    float getPixelValue(qreal x, qreal y, InterpolationMethod interpolation = linear) const;

    /**
     * @return slice plane's x-Axis vector in 3D space.
     */
    Q_INVOKABLE
    QVector3D xAxis() const
        {return this->cuttingPlane.tangent();}

    /**
     * @return slice plane's y-Axis vector in 3D space.
     */
    Q_INVOKABLE
    QVector3D yAxis() const
        {return this->cuttingPlane.cotangent();}

    /**
     * @return slice plane's normal vector.
     */
    Q_INVOKABLE
    QVector3D normal() const
        {return this->cuttingPlane.normal();}

    /**
     * @return slice plane's origin in 3D space.
     */
    Q_INVOKABLE
    QVector3D origin() const
        {return this->cuttingPlane.origin;}

    /**
     * @brief shifts slice plane's origin to another point on the plane.
     * @param planePoint point on this slice's plane.
     */
    void setOrigin(const QPointF &planePoint);

    /**
     * @brief sets slice plane's origin to given point in 3D space.
     * @param origin
     */
    void setOrigin(const QVector3D& origin);

    /**
     * @brief sets slice plane's origin to given point in 3D space.
     * @param x
     * @param y
     * @param z
     */
    Q_INVOKABLE
    void setOrigin(qreal x, qreal y, qreal z)
        {setOrigin(QVector3D(x,y,z));}

    /**
     * @return this slice's rotation.
     */
    Q_INVOKABLE
    QQuaternion rotation() const
        {return this->cuttingPlane.rotation;}

    /**
     * @brief sets this slice's rotation
     * @param rotation
     */
    void setRotation(const QQuaternion& rotation);

    /**
      * @brief sets this slice's rotation
      * @param scalar part of quaternion
      * @param x vector part of quaternion
      * @param y vector part of quaternion
      * @param z vector part of quaternion
      */
    Q_INVOKABLE
    void setRotation(qreal scalar, qreal x, qreal y, qreal z)
        {this->setRotation(QQuaternion(scalar,x,y,z));}

    /**
     * @brief rotates plane around given axis by degrees
     * @see rotateAroundAxis(qreal, const QVector3D&)
     */
    Q_INVOKABLE
    void rotateAroundAxis(qreal degrees, qreal x, qreal y, qreal z);

    /**
     * @brief rotates plane around given axis by degrees
     * @param degress angle in degrees
     * @param axis to rotate around
     */
    Q_INVOKABLE
    void rotateAroundAxis(qreal degrees, const QVector3D& axis)
        {this->rotateAroundAxis(degrees, axis.x(),axis.y(),axis.z());}

    /**
     * @brief moves origin in direction of normal by delta
     * @param delta distance to move
     */
    Q_INVOKABLE
    void translateAlongNormal(qreal delta);

    /**
     * @brief calculates a rectangle on the slice that contains the intersection of
     * the slices plane with the dataset volume
     * @return bounding rectangle on plane that contains whole intersection with volume
     */
    Q_INVOKABLE
    QRectF getBoundingRectangle() const;


    // throws ScriptingException
    static Slice* getTestSlice()
        {return new Slice(DataSet::getTestDataSet());}

    /**
     * @brief generates image from intersecting voxels with plane in give area.
     * @param sliceArea area on plane from which the image is created
     * @param imageSize size of resulting image (should match sliceAreas aspect ratio for undistorted image)
     * @param interpolation either linear or nearestNeighbor
     */
    Q_INVOKABLE
    SliceImage generateImage(const QRectF& sliceArea, const QSize& imageSize, InterpolationMethod interpolation = linear) const;

    // throws ScriptingException
    static SliceImage generateTestImage(int width = 200, int height = 200);

    /**
     * @brief saves image of slice to file (e.g. jpg or png)
     * @param file filepath where image should be saved to
     * @param width images width
     * @param height images height
     * @param interpolation when true image generating will use linear interpolation
     * @param lowestValue all voxels below this value will be black
     * @param highestValue all voxels above this value will be white
     * @see saveSliceImage(QString, QSize, QRectF, bool, qreal, qreal)
     * @return true on success
     */
    Q_INVOKABLE
    bool saveSliceImage(QString file, uint width, uint height, bool interpolation = true, qreal lowestValue = 0, qreal highestValue = 1) const;

    /**
     * @brief saves image of slicearea to file (e.g. jpg or png)
     * @param file filepath where image should be saved to
     * @param imageSize size of image
     * @param sliceArea area from which image is generated
     *  (area will be expanded to fit image size aspect ratio if necessary)
     * @param interpolation when true image generating will use linear interpolation
     * @param lowestValue all voxels below this value will be black
     * @param highestValue all voxels above this value will be white
     * @return true on success
     */
    Q_INVOKABLE
    bool saveSliceImage(QString file, QSize imageSize, QRectF sliceArea, bool interpolation = true, qreal lowestValue = 0, qreal highestValue = 1) const;

signals:
    /**
     * @brief emited when this slice's plane changed (rotated or origin moved).
     * @param oldPlane
     * @param newPlane
     * @param equivalent is true when old and new plane are identical (same normal and new origin on old plane).
     */
    void planeChanged(Plane oldPlane, Plane newPlane, bool equivalent);

private:
    Plane cuttingPlane;
    DataSet* dataset;

};

namespace internal {
class SliceAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Slice")

    Slice* object;

public:
    SliceAdaptor (Slice* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~SliceAdaptor () {}

    Q_PROPERTY (QDBusObjectPath/* voxie::data::DataSet* */ DataSet READ dataSet)
    QDBusObjectPath/* voxie::data::DataSet* */ dataSet();

    Q_PROPERTY (voxie::scripting::Plane Plane READ plane WRITE setPlane)
    voxie::scripting::Plane plane();
    void setPlane(const voxie::scripting::Plane& plane);

    Q_PROPERTY (QString DisplayName READ displayName)
    QString displayName ();

public slots:
    
};
}

}}

Q_DECLARE_METATYPE(voxie::data::Slice*)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
