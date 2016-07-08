#pragma once

#include <PluginVis3D/surfaceextractor.hpp>

#include <QtGui/QVector3D>

class MarchingCubes : public SurfaceExtractor {
    Q_OBJECT

public:
    MarchingCubes(QObject* parent = nullptr);
    ~MarchingCubes();

    virtual QSharedPointer<voxie::data::Surface> extract(const QSharedPointer<voxie::io::Operation>& operation, const QSharedPointer<voxie::data::VoxelData>& data, float threshold, bool invert);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
