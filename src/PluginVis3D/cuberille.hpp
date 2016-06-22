#pragma once

#include <PluginVis3D/surfaceextractor.hpp>

class Cuberille : public SurfaceExtractor {
    Q_OBJECT

public:
    Cuberille(QObject* parent = nullptr);
    ~Cuberille();

    virtual QSharedPointer<voxie::data::Surface> extract(voxie::io::Operation* operation, voxie::data::VoxelData* data, float threshold, bool invert);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
