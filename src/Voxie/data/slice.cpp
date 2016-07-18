#include "slice.hpp"

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/opencl/clinstance.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QTime>

#include <QtGui/QMatrix4x4>

#include <QtGui/QIcon>

using namespace voxie::data;
using namespace voxie::data::internal;
int sliceCount = 0;

Slice::Slice(DataSet* parent) : DataObject("Slice", parent), dataset(parent)
{
    new SliceAdaptor (this);

    Q_ASSERT(parent != nullptr); this->setParent(parent);

    this->setDisplayName(QString("Slice") + QString::number(sliceCount));
    sliceCount++;

    this->cuttingPlane.origin = parent->volumeCenter();
}

QIcon Slice::icon() const {
    return QIcon(":/icons/layer.png");
}


float
Slice::getPixelValue(qreal x, qreal y, InterpolationMethod interpolation) const
{
    QVector3D p = this->cuttingPlane.get3DPoint(x,y);
    return this->dataset->filteredData()->getVoxelMetric(p.x(), p.y(), p.z(), interpolation);
}


void
Slice::setOrigin(const QPointF &planePoint)
{
    QVector3D origin = this->cuttingPlane.get3DPoint(planePoint.x(), planePoint.y());
    if(origin == this->cuttingPlane.origin){
        return;
    }
    Plane old = this->cuttingPlane;
    this->cuttingPlane.origin = origin;

    emit this->planeChanged(old, this->cuttingPlane, true);
}

void
Slice::setOrigin(const QVector3D &origin)
{
    if(origin == this->cuttingPlane.origin){
        return;
    }

    Plane old = this->cuttingPlane;
    this->cuttingPlane.origin = origin;

    emit this->planeChanged(old, this->cuttingPlane, old.isOnPlane(origin));
}

void
Slice::setRotation(const QQuaternion &rot)
{
    QQuaternion rotation = rot.normalized();
    if(rotation == this->cuttingPlane.rotation){
        return;
    }

    Plane old = this->cuttingPlane;
    this->cuttingPlane.rotation = rotation;

    emit this->planeChanged(old, this->cuttingPlane, (old.normal() - this->cuttingPlane.normal()).lengthSquared() < 1e-8 );
}

void Slice::setPlane(const Plane& plane) {
    QQuaternion rotation = plane.rotation.normalized();

    if (plane.origin == this->cuttingPlane.origin && rotation == this->cuttingPlane.rotation) {
        return;
    }

    Plane old = this->cuttingPlane;
    this->cuttingPlane.origin = plane.origin;
    this->cuttingPlane.rotation = rotation;

    emit this->planeChanged(old, this->cuttingPlane, (old.normal() - this->cuttingPlane.normal()).lengthSquared() < 1e-8 && old.isOnPlane(plane.origin));
}

void
Slice::rotateAroundAxis(qreal degrees, qreal x, qreal y, qreal z)
{
    QQuaternion rotation = QQuaternion::fromAxisAndAngle(x,y,z, degrees);
    rotation = rotation * this->rotation();
    this->setRotation(rotation);
}

void
Slice::translateAlongNormal(qreal delta)
{
    if(delta != 0){
        this->setOrigin(this->origin() + (this->normal()*delta) );
    }
}


QRectF
Slice::getBoundingRectangle() const
{
    QVector<QVector3D> polygon3D = this->cuttingPlane.intersectVolume(-this->dataset->filteredData()->getFirstVoxelPosition(), this->dataset->filteredData()->getDimensionsMetric());
    if(polygon3D.length() < 2){
        // no intersection with volume
        return QRectF(0,0,0,0);
    }

    QVector3D x = this->xAxis();
    QVector3D y = this->yAxis();
    QVector3D z = this->normal();
    QVector3D p = this->origin();

    // set up transformation matrices
    QMatrix4x4 translation; translation.setToIdentity();
    translation.translate(p);
    QMatrix4x4 rotation; rotation.setToIdentity();
    rotation.setColumn(0, x.toVector4D());
    rotation.setColumn(1, y.toVector4D());
    rotation.setColumn(2, z.toVector4D());

    QMatrix4x4 transform = translation * rotation;
    transform = transform.inverted();

    // find bounding rectangle
    qreal left, right, top, bottom;
    left = right = (transform * polygon3D[0]).x();
    top = bottom = (transform * polygon3D[0]).y();
    for(int i = 1; i < polygon3D.length(); i++){
        // transform into plane coordinate system
        QVector3D transformed = transform * polygon3D[i];
        if(transformed.x() > right){
            right = transformed.x();
        } else if(transformed.x() < left){
            left = transformed.x();
        }

        if(transformed.y() > bottom){
            bottom = transformed.y();
        } else if(transformed.y() < top){
            top = transformed.y();
        }
    }

    return QRectF(left, top, right-left, bottom-top);
}


SliceImage
Slice::generateImage(
        const QRectF &sliceArea,
        const QSize &imageSize,
        InterpolationMethod interpolation) const
{
    return generateImage(this->getDataset()->filteredData().data(), this->getCuttingPlane(), sliceArea, imageSize, interpolation);
}

SliceImage
Slice::generateImage(
        VoxelData* data,
        const Plane& plane,
        const QRectF &sliceArea,
        const QSize &imageSize,
        InterpolationMethod interpolation)
{
    SliceImageContext imgContext = {plane, sliceArea, data->getSpacing()};
    SliceImage img((size_t) imageSize.width(), (size_t) imageSize.height(), imgContext, false);

    QVector3D origin = plane.origin;
    origin += plane.tangent() * sliceArea.x();
    origin += plane.cotangent() * sliceArea.y();

    try {
        data->extractSlice(origin, plane.rotation, imageSize, sliceArea.width() / imageSize.width(), sliceArea.height() / imageSize.height(), interpolation, img);
    } catch (voxie::scripting::ScriptingException& e) {
        qWarning() << "Extracting slice failed";
    }

    return img;
}

SliceImage
Slice::generateTestImage(int width, int height)
{
    Slice* slice = Slice::getTestSlice();
    slice->setRotation(QQuaternion::fromAxisAndAngle(1,1,1,50));
    return slice->generateImage(slice->getBoundingRectangle(), QSize(width,height));
}


bool
Slice::saveSliceImage(QString file, uint width, uint height, bool interpolation, qreal lowestValue, qreal highestValue) const
{
    return this->saveSliceImage(file, QSize(width, height), getBoundingRectangle(), interpolation, lowestValue, highestValue);
}

bool
Slice::saveSliceImage(QString file, QSize imageSize, QRectF sliceArea, bool interpolation, qreal lowestValue, qreal highestValue) const
{
    if(imageSize.width() == 0 || imageSize.height() == 0 || sliceArea.width() == 0 || sliceArea.height() == 0){
        return false;
    }

    qreal aspectImg = (imageSize.width()*1.0)/imageSize.height();
    qreal aspectRect = sliceArea.width()/sliceArea.height();
    if(aspectImg > aspectRect){
        // img wider
        sliceArea.setWidth(aspectImg * sliceArea.height());
    } else {
        // img taller
        sliceArea.setHeight(sliceArea.width() / aspectImg);
    }

    return this->generateImage(sliceArea, imageSize,interpolation ? linear:nearestNeighbor).toQImage(lowestValue, highestValue).save(file);
}

QDBusObjectPath/* voxie::data::DataSet* */ SliceAdaptor::dataSet()
{
  return voxie::scripting::ScriptableObject::getPath(object->getDataset());
}

voxie::scripting::Plane SliceAdaptor::plane() {
    return object->getCuttingPlane();
}
void SliceAdaptor::setPlane(const voxie::scripting::Plane& plane) {
    // TODO: return error for non-normalized quaternion? Doesn't work because setting (or getting) a property won't set the DBus context
    //qDebug() << object->calledFromDBus(); // will print 'false'
    //object->sendErrorReply ("foo", "bar");

    object->setPlane(plane);
}

QString SliceAdaptor::displayName () {
    return object->displayName();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
