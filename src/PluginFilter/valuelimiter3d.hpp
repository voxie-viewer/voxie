#pragma once

#include <Voxie/filter/filter3d.hpp>

#include <Voxie/plugin/metafilter3d.hpp>

#include <QtCore/QObject>

class ValueLimiter3D : public voxie::filter::Filter3D
{
    Q_OBJECT
public:
    explicit ValueLimiter3D(QObject *parent = 0);

    ~ValueLimiter3D();

    virtual voxie::data::VoxelData* getSourceVolume(voxie::data::VoxelData* input) override;

    virtual void applyTo(voxie::data::VoxelData* input, voxie::data::VoxelData* output) override;

    virtual bool hasSettingsDialog() override {
        return true;
    }

    virtual QDialog* getSettingsDialog() override;

    virtual QXmlStreamAttributes exportFilterSettingsXML() override;

    virtual void importFilterSettingsXML(QXmlStreamAttributes attributes) override;

    float getLowerLimit(){return this->lowerLimit;}

    float getUpperLimit(){return this->upperLimit;}

    void setLowerLimit(float value);

    void setUpperLimit(float value);

    void setLimits(float upper, float lower);

private:
    float lowerLimit;
    float upperLimit;
    QDialog* settingsdialog;
};

class MetaValueLimiter3D : public voxie::plugin::MetaFilter3D
{
public:
    MetaValueLimiter3D(QObject* parent = 0) : MetaFilter3D(parent)
    {
        this->setObjectName("Value Limiter");
    }

    virtual voxie::filter::Filter3D *createFilter() const;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
