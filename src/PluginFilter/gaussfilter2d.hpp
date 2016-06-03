#pragma once

#include <Voxie/filter/filter2d.hpp>

#include <Voxie/plugin/metafilter2d.hpp>

#include <QtWidgets/QSpinBox>

class GaussFilter2D : public voxie::filter::Filter2D
{
    Q_OBJECT
    Q_DISABLE_COPY(GaussFilter2D)
public:
    GaussFilter2D(QObject* parent = 0);
    ~GaussFilter2D();

    virtual void applyTo(voxie::data::FloatImage input, voxie::data::FloatImage output) override;

    virtual void applyTo(voxie::data::SliceImage input, voxie::data::SliceImage output) override;

    virtual QDialog* getSettingsDialog() override;

	bool hasSettingsDialog() override {
		return true;
	}

    virtual QXmlStreamAttributes exportFilterSettingsXML() override;

    virtual void importFilterSettingsXML(QXmlStreamAttributes attributes) override;

private:
    virtual void updateSettings();
    virtual void calcGaussKernel();
    QDialog* dialog = nullptr;
    int radius = 2;
    int kernelSize = 0;
    float sigma = 0;
    QVector<float> gaussKernel;
    QSpinBox* spinBox = nullptr;
};


class GaussMetaFilter2D :  public voxie::plugin::MetaFilter2D
{
   public:
    GaussMetaFilter2D()
    {
        this->setObjectName("Gaussfilter2D");
    }
    virtual voxie::filter::Filter2D *createFilter() const;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
