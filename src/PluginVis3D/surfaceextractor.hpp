#pragma once

#include <QtCore/QObject>

namespace voxie { namespace data {
    class Surface;
    class SurfaceBuilder;
    class VoxelData;
} }

namespace voxie { namespace io {
    class Operation;
} }

class SurfaceExtractor : public QObject {
    Q_OBJECT

public:
    SurfaceExtractor(QObject* parent = nullptr);
    ~SurfaceExtractor();

    virtual QSharedPointer<voxie::data::Surface> extract(voxie::io::Operation* operation, voxie::data::VoxelData* data, float threshold, bool invert) = 0;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
