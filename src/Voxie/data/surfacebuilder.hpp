#pragma once

#include <Voxie/data/surface.hpp>

#include <QtCore/QMap>

namespace voxie { namespace data {

class VOXIECORESHARED_EXPORT SurfaceBuilder : public QObject {
    Q_OBJECT

        public:
    using IndexType = Surface::IndexType;
    using Triangle = Surface::Triangle;

 private:
    struct Vec3Wrapper {
        QVector3D data;
        Vec3Wrapper(){}
        Vec3Wrapper(const QVector3D& data) : data (data) {}
        operator QVector3D() const { return data; }

        bool operator<(const Vec3Wrapper& second) const {
            if (data.x() < second.data.x())
                return true;
            else if (data.x() > second.data.x())
                return false;

            if (data.y() < second.data.y())
                return true;
            else if (data.y() > second.data.y())
                return false;

            if (data.z() < second.data.z())
                return true;
            else if (data.z() > second.data.z())
                return false;

            return false;
        }
    };

    QVector<QVector3D> vertices_;
    QVector<Triangle> triangles_;
    QMap<Vec3Wrapper, IndexType> indices_;

 public:
    SurfaceBuilder(QObject* parent = nullptr);
    ~SurfaceBuilder();

    // TODO: this should merge vertices which are very close to each other
    IndexType addVertex(QVector3D vertex) {
        auto it = indices_.constFind(vertex);
        if (it != indices_.constEnd())
            return *it;

        IndexType index = vertices_.size();
        vertices_.push_back(vertex);
        indices_.insert(vertex, index);
        return index;
    }

    void addTriangle(QVector3D a, QVector3D b, QVector3D c) {
        triangles_.push_back({addVertex(a), addVertex(b), addVertex(c)});
    }

    void clear();

    // Will clear() the builder object
    QSharedPointer<Surface> createSurfaceClearBuilder();
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
