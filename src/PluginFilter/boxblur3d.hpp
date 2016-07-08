#pragma once

#include <Voxie/filter/filter3d.hpp>

#include <Voxie/plugin/metafilter3d.hpp>

class BoxBlur3D : public voxie::filter::Filter3D
{
    Q_OBJECT
public:
    BoxBlur3D(QObject* parent = 0);

    virtual QSharedPointer<voxie::data::VoxelData> getSourceVolume(const QSharedPointer<voxie::data::VoxelData>& input) override
        {return input;}// in-place filter

    virtual void applyTo(const QSharedPointer<voxie::data::VoxelData>& input, const QSharedPointer<voxie::data::VoxelData>& output) override;

    virtual bool hasSettingsDialog() override {
        return true;
    }

    virtual QDialog* getSettingsDialog() override;

    virtual QXmlStreamAttributes exportFilterSettingsXML() override;

    virtual void importFilterSettingsXML(QXmlStreamAttributes attributes) override;

    int getRadius() {return this->radius;}

    void setRadius(int radius);

private:
    QDialog* dialog;
    int radius;

};


class BoxBlur3DMeta :  public voxie::plugin::MetaFilter3D
{
    Q_OBJECT
public:
    BoxBlur3DMeta()
    {
        this->setObjectName("Box Blur");
    }

    virtual voxie::filter::Filter3D *createFilter() const;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
