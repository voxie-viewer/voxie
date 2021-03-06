#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtGui/QVector3D>

#include <array>

namespace voxie { namespace data {

class VOXIECORESHARED_EXPORT SurfaceBuilder;

class VOXIECORESHARED_EXPORT Surface : public QObject {
    Q_OBJECT

        friend class SurfaceBuilder;

 public:
    typedef quint32 IndexType;
    typedef std::array<IndexType, 3> Triangle;

    static const IndexType invalidIndex = -1;

 private:
    std::vector<QVector3D> vertices_;
    std::vector<Triangle> triangles_;

    Surface(const std::vector<QVector3D>&& vertices, const std::vector<Triangle>&& triangles);

 public:
    ~Surface();

    const std::vector<QVector3D>& vertices() const { return vertices_; }
    const std::vector<Triangle>& triangles() const { return triangles_; }
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
