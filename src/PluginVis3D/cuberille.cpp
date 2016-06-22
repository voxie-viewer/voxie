#include "cuberille.hpp"

#include <Voxie/data/surfacebuilder.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/io/operation.hpp>

#include <QtCore/QSharedPointer>

using namespace voxie::data;

Cuberille::Cuberille(QObject* parent) : SurfaceExtractor(parent) {
}
Cuberille::~Cuberille() {
}

static void addQuad(SurfaceBuilder* sb, const QVector3D &a, const QVector3D &b, const QVector3D &c, const QVector3D &d) {
    sb->addTriangle(a, b, c);
    sb->addTriangle(a, c, d);
}

static void genCube(const QVector3D &position, const QVector3D &size, int sides, SurfaceBuilder* sb) {
    QVector3D min = position - size;
    QVector3D max = position + size;

    // Front (+z)
    if (sides & 1) {
        addQuad(sb,
                QVector3D(min.x(), min.y(), max.z()),
                QVector3D(max.x(), min.y(), max.z()),
                QVector3D(max.x(), max.y(), max.z()),
                QVector3D(min.x(), max.y(), max.z()));
    }

    // Back (-z)
    if (sides & 2) {
        addQuad(sb,
                QVector3D(min.x(), min.y(), min.z()),
                QVector3D(min.x(), max.y(), min.z()),
                QVector3D(max.x(), max.y(), min.z()),
                QVector3D(max.x(), min.y(), min.z()));
    }

    // Left (+x)
    if (sides & 4) {
        addQuad(sb,
                QVector3D(max.x(), min.y(), min.z()),
                QVector3D(max.x(), max.y(), min.z()),
                QVector3D(max.x(), max.y(), max.z()),
                QVector3D(max.x(), min.y(), max.z()));
    }

    // Right (-x)
    if (sides & 8) {
        addQuad(sb,
                QVector3D(min.x(), min.y(), min.z()),
                QVector3D(min.x(), min.y(), max.z()),
                QVector3D(min.x(), max.y(), max.z()),
                QVector3D(min.x(), max.y(), min.z()));
    }

    // Top (+y)
    if (sides & 16) {
        addQuad(sb,
                QVector3D(min.x(), max.y(), min.z()),
                QVector3D(min.x(), max.y(), max.z()),
                QVector3D(max.x(), max.y(), max.z()),
                QVector3D(max.x(), max.y(), min.z()));
    }

    // Bottom (-y)
    if (sides & 32) {
        addQuad(sb,
                QVector3D(min.x(), min.y(), min.z()),
                QVector3D(max.x(), min.y(), min.z()),
                QVector3D(max.x(), min.y(), max.z()),
                QVector3D(min.x(), min.y(), max.z()));
    }
}

QSharedPointer<Surface> Cuberille::extract(voxie::io::Operation* operation, voxie::data::VoxelData* data, float threshold, bool invert) {
    QVector3D spacing = data->getSpacing();

    QVector3D size = 0.5f * spacing;

    auto dim = data->getDimensions();

    auto upper = dim;
    upper.x -= 1;
    upper.y -= 1;
    upper.z -= 1;

    QVector3D origin = data->getFirstVoxelPosition();

    QScopedPointer<SurfaceBuilder> sb(new SurfaceBuilder());

    for(size_t x = 0; x < dim.x; x++) {
        for(size_t y = 0; y < dim.y; y++) {
            for(size_t z = 0; z < dim.z; z++) {
                operation->throwIfCancelled();

                Voxel voxel = data->getVoxel(x, y, z);
                if((voxel < threshold) ^ invert)
                    continue;

                int majoraMask = 0xFF;
                if((x > 0) && ((data->getVoxel(x-1,y,z) >= threshold) ^ invert))
                    majoraMask &= ~8;
                if((y > 0) && ((data->getVoxel(x,y-1,z) >= threshold) ^ invert))
                    majoraMask &= ~32;
                if((z > 0) && ((data->getVoxel(x,y,z-1) >= threshold) ^ invert))
                    majoraMask &= ~2;
                if((x < upper.x) && ((data->getVoxel(x+1,y,z) >= threshold) ^ invert))
                    majoraMask &= ~4;
                if((y < upper.y) && ((data->getVoxel(x,y+1,z) >= threshold) ^ invert))
                    majoraMask &= ~16;
                if((z < upper.z) && ((data->getVoxel(x,y,z+1) >= threshold) ^ invert))
                    majoraMask &= ~1;

                if((majoraMask & 192) != 0) {
                    QVector3D pos = QVector3D(x * spacing.x(),
                                              y * spacing.y(),
                                              z * spacing.z())
                        + 0.5f * spacing + origin;
                    genCube(pos, size, majoraMask, sb.data());
                }
            }
            operation->updateProgress(1.0f * x / dim.x);
        }
    }

    return sb->createSurfaceClearBuilder();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
