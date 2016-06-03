#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QVector>

#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>

#include <QtWidgets/QWidget>

namespace voxie {
namespace filter {

/**
 * abstract class to define a 3D shape. Is used by Selection3DMask.
 */
class VOXIECORESHARED_EXPORT Shape3D : public QObject
{
    Q_OBJECT
public:
    Shape3D() : QObject(nullptr) {}
    virtual bool contains(qreal x, qreal y, qreal z) const = 0;
    bool contains(const QVector3D& point) const {return this->contains(point.x(),point.y(),point.z());}
    /** if the shapes volume is greater 0 -> true 3D shape */
    virtual bool isValid() {return true;}
    virtual Shape3D* copy() const = 0;
    /**
     * @return a widget for manipulating the shapes parameters
     */
    virtual QWidget* editorWidget() = 0;
    /** shapes name and overview of param values */
    virtual QString toString() = 0;
    /** shapes name */
    virtual QString name() = 0;

    /** shapes maximum extend in y direction */
    virtual qreal maxY() const = 0;
    /** shapes minimum extend in y direction */
    virtual qreal minY() const = 0;
    /** shapes minimum extend in x direction */
    virtual qreal minX() const = 0;
    /** shapes maximum extend in x direction */
    virtual qreal maxX() const = 0;
    /** shapes minimum extend in z direction */
    virtual qreal minZ() const = 0;
    /** shapes maximum extend in z direction */
    virtual qreal maxZ() const = 0;

    /**
     * @return a vector of all known different shape instances (Cuboid,Sphere,Tetrahedron)
     */
    static QVector<Shape3D*>& getShapeInstances() {return Shape3D::shapeInstances;}
signals:
    /** emmited when shape changes */
    void changed();
private:
    static QVector<Shape3D*> shapeInstances;
};

/**
 * A Coboid Shape
 */
class VOXIECORESHARED_EXPORT Cuboid: public Shape3D
{
    Q_OBJECT
public:
    Cuboid() : origin(0,0,0), dimension(1,1,1) {}
    Cuboid(QVector3D origin, QVector3D dimension) : origin(origin), dimension(dimension){}
    Cuboid(qreal x, qreal y, qreal z, qreal w, qreal h, qreal d) : origin(x,y,z), dimension(w,h,d) {}

    virtual bool contains(qreal x, qreal y, qreal z) const override {
        x -= origin.x();
        y -= origin.y();
        z -= origin.z();
        return  x >= 0 && x < dimension.x() &&
                y >= 0 && y < dimension.y() &&
                z >= 0 && z < dimension.z();
    }

    virtual bool isValid() override{
        return dimension.length() != 0;
    }

    virtual Shape3D* copy() const override {
        return new Cuboid(this->origin, this->dimension);
    }

    virtual qreal maxY() const override {return origin.y()+dimension.y();}
    virtual qreal minY() const override {return origin.y();}
    virtual qreal minX() const override {return origin.x();}
    virtual qreal maxX() const override {return origin.x()+dimension.x();}
    virtual qreal minZ() const override {return origin.z();}
    virtual qreal maxZ() const override {return origin.z()+dimension.z();}

    virtual QWidget* editorWidget() override;
    virtual QString toString() override;
    virtual QString name() override {return QString("Cuboid");}

    QVector3D getOrigin()const {return this->origin;}
    QVector3D getDimension()const {return this->dimension;}
    void setOrigin(const QVector3D& v)
    {if(v==this->origin){return;} this->origin = v; emit this->changed();}
    void setDimension(const QVector3D& v)
    {if(v==this->dimension){return;} this->dimension = v; emit this->changed();}

private:
    QVector3D origin;
    QVector3D dimension;
};

/**
 * A Tedrahedron shape
 */
class VOXIECORESHARED_EXPORT Tetrahedron: public Shape3D
{
    Q_OBJECT
public:
    Tetrahedron() : p0(1,0,0), p1(0,1,0), p2(0,0,1), p3(0,0,0) {}
    Tetrahedron(QVector3D p0, QVector3D p1, QVector3D p2, QVector3D p3) : p0(p0), p1(p1), p2(p2), p3(p3) {}
    Tetrahedron(qreal x0, qreal y0, qreal z0, qreal x1, qreal y1, qreal z1,  qreal x2, qreal y2, qreal z2,  qreal x3, qreal y3, qreal z3) : p0(x0,y0,z0), p1(x1,y1,z1), p2(x2,y2,z2), p3(x3,y3,z3) {}

    virtual bool contains(qreal x, qreal y, qreal z) const override {
        QVector3D pIn(x,y,z);
        qreal det = qmatrix4x4(p0,p1,p2,p3).determinant();
        qreal d0 = qmatrix4x4(pIn,p1,p2,p3).determinant();
        qreal d1 = qmatrix4x4(p0,pIn,p2,p3).determinant();
        qreal d2 = qmatrix4x4(p0,p1,pIn,p3).determinant();
        qreal d3 = qmatrix4x4(p0,p1,p2,pIn).determinant();
        if(det < 0){
            return d0 <= 0 && d1 <= 0 && d2 <= 0 && d3 <= 0;
        } else if(det > 0){
            return d0 >= 0 && d1 >= 0 && d2 >= 0 && d3 >= 0;
        } else {
            return false;
        }
    }

    virtual bool isValid() override {
        return qmatrix4x4(p0,p1,p2,p3).determinant() != 0;
    }

    virtual Shape3D* copy() const override {
        return new Tetrahedron(p0,p1,p2,p3);
    }

    virtual qreal maxY() const override {return max(1);}
    virtual qreal minY() const override {return min(1);}
    virtual qreal minX() const override {return min(0);}
    virtual qreal maxX() const override  {return max(0);}
    virtual qreal minZ() const override  {return min(2);}
    virtual qreal maxZ() const override  {return max(2);}

    virtual QWidget* editorWidget() override;

    virtual QString toString() override;
    virtual QString name() override {return QString("Tetrahedron");}

    void setVertex(int i, const QVector3D & v){
        switch (i) {
        case 0:
            if(v == p0){return;}
            p0 = v; emit this->changed();
            break;
        case 1:
            if(v == p1){return;}
            p1 = v; emit this->changed();
            break;
        case 2:
            if(v == p2){return;}
            p2 = v; emit this->changed();
            break;
        case 3:
            if(v == p3){return;}
            p3 = v; emit this->changed();
            break;
        default:
            break;
        }
    }

    QVector<QVector3D> getVertices() const {
        QVector<QVector3D> verts(4);
        verts[0] = p0; verts[1] = p1; verts[2] = p2; verts[3] = p3;
        return verts;
    }

private:
    QVector3D p0;
    QVector3D p1;
    QVector3D p2;
    QVector3D p3;

    static QMatrix4x4 qmatrix4x4(const QVector3D &v0, const QVector3D &v1, const QVector3D &v2, const QVector3D &v3){
        return QMatrix4x4(
                    v0[0],v0[1],v0[2], 1,
                    v1[0],v1[1],v1[2], 1,
                    v2[0],v2[1],v2[2], 1,
                    v3[0],v3[1],v3[2], 1
                );
    }

    qreal max(int i) const {
        QVector3D verts[3] = {p1,p2,p3}; qreal max = p0[i];
        for(int j = 0; j < 3; j++)
            max = verts[j][i] > max ? verts[j][i] : max;
        return max;
    }
    qreal min(int i) const {
        QVector3D verts[3] = {p1,p2,p3}; qreal min = p0[i];
        for(int j = 0; j < 3; j++)
            min = verts[j][i] < min ? verts[j][i] : min;
        return min;
    }
};

/**
 * A Sphere shape
 */
class VOXIECORESHARED_EXPORT Sphere: public Shape3D
{
    Q_OBJECT
public:
    Sphere() : origin(0,0,0), radius(1) {}
    Sphere(QVector3D origin, qreal radius) : origin(origin), radius(fabs(radius)){}
    Sphere(qreal x, qreal y, qreal z, qreal radius) : origin(x,y,z), radius(fabs(radius)) {}

    virtual bool contains(qreal x, qreal y, qreal z) const override {
        return (this->origin - QVector3D(x,y,z)).length() <= radius;
    }

    virtual bool isValid() override{
        return radius > 0;
    }

    virtual Shape3D* copy() const override {
        return new Sphere(this->origin, this->radius);
    }

    virtual qreal maxY() const override {return origin.y()+radius;}
    virtual qreal minY() const override {return origin.y()-radius;}
    virtual qreal minX() const override {return origin.x()-radius;}
    virtual qreal maxX() const override {return origin.x()+radius;}
    virtual qreal minZ() const override {return origin.z()-radius;}
    virtual qreal maxZ() const override {return origin.z()+radius;}

    virtual QWidget* editorWidget() override;
    virtual QString toString() override;
    virtual QString name() override {return QString("Sphere");}

    QVector3D getOrigin()const {return this->origin;}
    qreal getRadius()const {return this->radius;}
    void setOrigin(const QVector3D& v)
    {if(v==this->origin){return;} this->origin = v; emit this->changed();}
    void setRadius(qreal r)
    {if(r==this->radius){return;} this->radius = r; emit this->changed();}

private:
    QVector3D origin;
    qreal radius;
};


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
