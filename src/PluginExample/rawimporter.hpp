#pragma once

#include <Voxie/io/loader.hpp>

class RAWImporter :
        public voxie::io::Loader
{
    Q_OBJECT
public:
    explicit RAWImporter(QObject *parent = 0);

    virtual QSharedPointer<voxie::data::VoxelData> load(const QSharedPointer<voxie::io::Operation>& op, const QString &fileName) override;

signals:

public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
