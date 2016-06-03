#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/io/voxelexporter.hpp>

class HDFExporter : public voxie::io::VoxelExporter
{
    Q_OBJECT
    Q_DISABLE_COPY(HDFExporter)
private:
public:
    explicit HDFExporter(QObject *parent = 0);
    ~HDFExporter();

    virtual void exportGui(voxie::data::DataSet *dataSet) override;

    Q_INVOKABLE virtual void write(const QString &fileName, voxie::data::DataSet* dataSet);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
