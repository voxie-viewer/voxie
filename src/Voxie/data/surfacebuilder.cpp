#include "surfacebuilder.hpp"

#include <QtCore/QSharedPointer>

using namespace voxie::data;

SurfaceBuilder::SurfaceBuilder(QObject* parent) : QObject(parent) {
}
SurfaceBuilder::~SurfaceBuilder() {
}

void SurfaceBuilder::clear() {
    triangles_.clear();
    vertices_.clear();
    //indices_.clear();
}

QSharedPointer<Surface> SurfaceBuilder::createSurfaceClearBuilder() {
    QSharedPointer<Surface> surface(new Surface(std::move(vertices_), std::move(triangles_)), [](QObject* obj) { obj->deleteLater(); });
    clear();
    return surface;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
