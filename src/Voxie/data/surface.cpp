#include "surface.hpp"

using namespace voxie::data;

Surface::Surface(const QVector<QVector3D>&& vertices, const QVector<Triangle>&& triangles) : vertices_(vertices), triangles_(triangles) {
}
Surface::~Surface() {
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
