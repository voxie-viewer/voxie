#include "surface.hpp"

using namespace voxie::data;

Surface::Surface(const std::vector<QVector3D>&& vertices, const std::vector<Triangle>&& triangles) : vertices_(vertices), triangles_(triangles) {
}
Surface::~Surface() {
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
