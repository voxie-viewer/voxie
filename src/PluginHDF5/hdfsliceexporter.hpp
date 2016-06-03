#pragma once

#include <Voxie/data/slice.hpp>

#include <Voxie/io/sliceexporter.hpp>

class HDFSliceExporter : public voxie::io::SliceExporter
{
    Q_OBJECT
    Q_DISABLE_COPY(HDFSliceExporter)
private:
public:
    explicit HDFSliceExporter(QObject *parent = 0);
    ~HDFSliceExporter();

    virtual void exportGui(voxie::data::Slice *slice) override;

    Q_INVOKABLE void exportSlice(QString fileName, voxie::data::Slice *slice, float x, float y, float xMax, float yMax, float sizeX, float sizeY, voxie::data::InterpolationMethod interpolation);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
