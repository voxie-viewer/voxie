#pragma once

#include <Voxie/io/loader.hpp>

class RAWImporter :
        public voxie::io::Loader
{
    Q_OBJECT
public:
    explicit RAWImporter(QObject *parent = 0);

    virtual voxie::data::VoxelData* loadImpl(const QString &fileName) override;

signals:

public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
