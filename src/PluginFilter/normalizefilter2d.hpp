#pragma once

#include <Voxie/filter/filter2d.hpp>

#include <Voxie/plugin/metafilter2d.hpp>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>

class NormalizeFilter2D : public voxie::filter::Filter2D
{
    Q_OBJECT
    Q_DISABLE_COPY(NormalizeFilter2D)
public:
    NormalizeFilter2D(QObject* parent = 0);
    ~NormalizeFilter2D();

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
    virtual void updateDialog();
    float lowerLimit = 0.0;
    float upperLimit = 1.0;
    bool autoLimits = true;
    QDoubleSpinBox* minBox = nullptr;
    QDoubleSpinBox* maxBox = nullptr;
    QCheckBox* autoBox = nullptr;
    QDialog* dialog = nullptr;
};


class NormalizeMetaFilter2D :  public voxie::plugin::MetaFilter2D
{
   public:
    NormalizeMetaFilter2D()
    {
        this->setObjectName("Normalizefilter2D");
    }

    virtual voxie::filter::Filter2D *createFilter() const;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
