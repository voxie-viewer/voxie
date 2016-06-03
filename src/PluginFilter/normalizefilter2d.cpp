#include "normalizefilter2d.hpp"

#include <Voxie/plugin/metafilter2d.hpp>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>

using namespace voxie::data;
using namespace voxie::plugin;

NormalizeFilter2D::NormalizeFilter2D(QObject *parent) :
    Filter2D(parent)
{
}

NormalizeFilter2D::~NormalizeFilter2D()
{

}

void NormalizeFilter2D::applyTo(voxie::data::FloatImage input, voxie::data::FloatImage output)
{
    float min = this->lowerLimit;
    float max = this->upperLimit;

    if(this->autoLimits){
        auto minmax = input.getMinMaxValue();
        this->lowerLimit = min = minmax.first;
        this->upperLimit = max = minmax.second;
    }

    // force cpu
    input.switchMode(FloatImage::STDMEMORY_MODE);
    output.switchMode(FloatImage::STDMEMORY_MODE);
    for(size_t i = 0; i < output.getHeight()*output.getWidth(); i++) {
        float v = input.getBuffer()[i];
        if(max == min) {
            v = 0.0f;
        }
        else{
            v = (v - min) / (max - min);
        }
        output.getBuffer()[i] = v;
    }
}

void NormalizeFilter2D::applyTo(voxie::data::SliceImage input, voxie::data::SliceImage output)
{
    applyTo((FloatImage)input, (FloatImage)output);
}

QDialog* NormalizeFilter2D::getSettingsDialog()
{
    if (this->dialog == nullptr) {
        this->dialog = new QDialog();
        this->dialog->setLayout(new QBoxLayout(QBoxLayout::LeftToRight));
        //dialog->layout()->addWidget(new QLabel("auto:"));
        this->autoBox = new QCheckBox("automatic \nlimits");
        this->autoBox->setChecked(this->autoLimits);
        this->dialog->layout()->addWidget(autoBox);
        this->dialog->layout()->addWidget(new QLabel("min:"));
        this->minBox = new QDoubleSpinBox();
        this->minBox->setValue(this->lowerLimit);
        this->minBox->setMinimum(std::numeric_limits<int>::lowest());
        this->minBox->setMaximum(std::numeric_limits<int>::max());
        this->dialog->layout()->addWidget(minBox);
        this->dialog->layout()->addWidget(new QLabel("max:"));
        this->maxBox = new QDoubleSpinBox();
        this->maxBox->setValue(this->upperLimit);
        this->maxBox->setMinimum(std::numeric_limits<int>::lowest());
        this->maxBox->setMaximum(std::numeric_limits<int>::max());
        this->dialog->layout()->addWidget(maxBox);
        connect(this->autoBox, &QCheckBox::stateChanged, this, &NormalizeFilter2D::updateDialog);
        void (QDoubleSpinBox:: *signal)(double) = &QDoubleSpinBox::valueChanged;
        connect(this->minBox, signal, this, &NormalizeFilter2D::updateSettings);
        connect(this->maxBox, signal, this, &NormalizeFilter2D::updateSettings);
    }
    updateDialog();
    return this->dialog;
}

void NormalizeFilter2D::updateDialog()
{
    if (this->autoBox->isChecked()) {
        this->autoLimits = true;
        this->minBox->setDisabled(true);
        this->maxBox->setDisabled(true);
    } else {
        this->autoLimits = false;
        this->lowerLimit = this->minBox->value();
        this->upperLimit = this->maxBox->value();
        this->minBox->setDisabled(false);
        this->maxBox->setDisabled(false);
    }
    emit filterChanged(this);
}

void NormalizeFilter2D::updateSettings()
{
    this->lowerLimit = this->minBox->value();
    this->upperLimit = this->maxBox->value();
    emit filterChanged(this);
}

QXmlStreamAttributes NormalizeFilter2D::exportFilterSettingsXML()
{
    QXmlStreamAttributes attributes;
    attributes.append("autoLimits", QString::number(this->autoLimits));
    attributes.append("lowerLimit", QString::number(this->lowerLimit));
    attributes.append("upperLimit", QString::number(this->upperLimit));
    return attributes;
}

void NormalizeFilter2D::importFilterSettingsXML(QXmlStreamAttributes attributes)
{
    this->autoLimits = attributes.value("autoLimits").toInt();
    this->lowerLimit = attributes.value("lowerLimit").toFloat();
    this->upperLimit = attributes.value("upperLimit").toFloat();
}

Filter2D* NormalizeMetaFilter2D::createFilter() const{
    NormalizeFilter2D* filter = new NormalizeFilter2D();
    //filter->setName("Normalize Filter");
    filter->setMetaName(this->objectName());
    return filter;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
