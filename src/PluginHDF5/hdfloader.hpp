#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/io/loader.hpp>

class HDFLoader : public voxie::io::Loader
{
    Q_OBJECT
    Q_DISABLE_COPY(HDFLoader)

public:
    explicit HDFLoader(QObject *parent = 0);
    ~HDFLoader();

    virtual QSharedPointer<voxie::data::VoxelData> loadImpl(const QString &fileName) override;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
