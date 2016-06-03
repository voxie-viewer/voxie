#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/filtermask/shape3d.hpp>

#include <Voxie/scripting/dbustypes.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtGui/QVector3D>

namespace voxie {
namespace filter {

/**
 * Slection3DMask holds a list of Shape3D objects. A Filter3D object uses
 * such a mask to determine on which Voxels the filter fill be applied.
 * The shapes coordinates are metric so masks are compatible with lower resolution
 * VoxelData objects of same metric size.
 */
class VOXIECORESHARED_EXPORT Selection3DMask: public QObject
{
    Q_OBJECT
public:
    Selection3DMask(QObject* parent) : QObject(parent), boundingBox(0,0,0,0,0,0) {}
    ~Selection3DMask();

    QVector<Shape3D*> allShapes() {return this->selectionShapes;}
    void addShape(Shape3D *shape);
    void removeShapeAt(int index);
    bool removeShape(Shape3D* shape);
    Shape3D* getShape(int index) const;
    void clear();
    int numShapes() const {return this->selectionShapes.length();}
    bool isEmpty() const {return this->selectionShapes.isEmpty();}

    /**
     * tests if a nonmetric point (voxel position) p is contained by this mask.
     * @param p voxel position
     * @param gridspacing voxel grid spacing of the voxels dataset (see VoxelData::getSpacing())
     * @param origin of the voxels dataset (see VoxelData::getOrigin())
     * @return true if contained
     */
    inline bool contains(const voxie::scripting::IntVector3 &p, const QVector3D &gridspacing, const QVector3D &origin) const {
        return contains(p.x,p.y,p.z,gridspacing,origin);
    }

    /**
     * tests if a nonmetric point (voxel position) (x,y,z) is contained by this mask.
     * @param x coord
     * @param y coord
     * @param z coord
     * @param gridspacing voxel grid spacing of the voxels dataset (see VoxelData::getSpacing())
     * @param origin of the voxels dataset (see VoxelData::getOrigin())
     * @return true if contained
     */
    inline bool contains(size_t x, size_t y, size_t z, const QVector3D &gridspacing, const QVector3D &origin) const {
        return containsMetric(x*gridspacing.x() - origin.x(), y*gridspacing.y() - origin.y(), z*gridspacing.z() - origin.z());
    }

    /**
     * tests if a metric position is contained by this mask
     * @param p metic point
     * @return true if is contained
     */
    inline bool containsMetric(const QVector3D& p) const {return containsMetric(p.x(), p.y(), p.z());}
    /**
     * tests if a metric position (x,y,z) is contained by this mask
     * @param x
     * @param y
     * @param z
     * @return true if is contained
     */
    inline bool containsMetric(qreal x, qreal y, qreal z) const {
        if(isEmpty()){
            return true; // when empty every point is contained in mask
        } else if(!boundingBox.contains(x,y,z)){
            return false;
        } else {
            for(const Shape3D* shape: this->selectionShapes){
                if(shape->contains(x,y,z))
                    return true;
            }
            return false;
        }
    }

signals:
    /**
     * emmited when shape is added/removed or when a shape changes
     */
    void changed();
private:
    QVector<Shape3D*> selectionShapes;
    Cuboid boundingBox;

    void calcBoundingbox();
    void shapeChanged();
};


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
