#include "gaussfilter2d.hpp"

#include <Voxie/plugin/metafilter2d.hpp>

#include <math.h>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

using namespace voxie::data;
using namespace voxie::plugin;

GaussFilter2D::GaussFilter2D(QObject *parent) :
    Filter2D(parent)
{
}

GaussFilter2D::~GaussFilter2D()
{

}

void GaussFilter2D::calcGaussKernel() {
    this->sigma = ((float)(this->radius*2)+1)/3;
    this->kernelSize = this->radius * 2 + 1;
    this->gaussKernel = QVector<float>(this->kernelSize*this->kernelSize);

    float kernelSum = 0; //for normalization

    float factor = 1 / (2 * M_PI * this->sigma * this->sigma);
    for(int x=0; x<this->kernelSize; x++) {
        for (int y=0; y<this->kernelSize; y++) {
            int xVal = kernelSize/2 - x;
            int yVal = kernelSize/2 - y;
            float expVal = -(xVal*xVal + yVal*yVal) / (2 * this->sigma * this->sigma);
            this->gaussKernel[x + this->kernelSize*y] = factor * exp(expVal);
            kernelSum += factor * exp(expVal);
        }
    }
    //normalize kernel
    float invSum = 1/kernelSum;
    for(int x=0; x<this->kernelSize; x++) {
        for (int y=0; y<this->kernelSize; y++) {
            this->gaussKernel[x + this->kernelSize*y] *= invSum;
        }
    }
}

void GaussFilter2D::applyTo(voxie::data::FloatImage input, voxie::data::FloatImage output)
{
    calcGaussKernel();
    for (size_t x=0; x<input.getWidth(); x++) {
        for (size_t y=0; y<input.getHeight(); y++) {
            float value = 0;
            if ( std::isnan(input.getPixel(x, y)) ) { //NaN
                output.setPixel(x,y, NAN);
                continue;
            }
            float scaling = this->kernelSize * this->kernelSize; // to balance NaNs
            for(size_t i=0; i < (size_t) this->kernelSize; i++) {
                for(size_t j=0; j < (size_t) this->kernelSize; j++) {
                    long long xPos = x + i - kernelSize/2;
                    long long yPos = y + j - kernelSize/2;
                    if (xPos >= 0 && xPos < (long long)input.getWidth() && yPos >= 0 && yPos < (long long)input.getHeight()) {
                        float pixelVal = input.getPixel(xPos, yPos);
                        if (!std::isnan(pixelVal)) { //checking NAN
                            value += this->gaussKernel[(int)(i+j*this->kernelSize)] * pixelVal;
                        } else {
                            scaling -= 1.0;
                        }
                    }
                }
            }
            scaling = float(this->kernelSize*this->kernelSize)/scaling;
            output.setPixel(x, y, value * scaling);
        }
    }
}

void GaussFilter2D::applyTo(voxie::data::SliceImage input, voxie::data::SliceImage output)
{
    applyTo((FloatImage)input, (FloatImage)output);
}

QDialog* GaussFilter2D::getSettingsDialog()
{
    if (this->dialog == nullptr) {
        this->dialog = new QDialog();
        this->dialog->setLayout(new QBoxLayout(QBoxLayout::LeftToRight));
        this->dialog->layout()->addWidget(new QLabel("radius:"));
        this->spinBox = new QSpinBox();
        this->spinBox->setValue(this->radius);
        this->dialog->layout()->addWidget(this->spinBox);
        QPushButton* button = new QPushButton("Update");
        this->dialog->layout()->addWidget(button);
        connect(button, &QPushButton::clicked, this, &GaussFilter2D::updateSettings);
    }
    return dialog;
}

void GaussFilter2D::updateSettings()
{
    if (this->spinBox != nullptr) {
        this->radius = this->spinBox->value();
    }
    emit this->filterChanged(this);
}

QXmlStreamAttributes GaussFilter2D::exportFilterSettingsXML()
{
    QXmlStreamAttributes attributes;
    attributes.append("radius", QString::number(this->radius));
    return attributes;
}

void GaussFilter2D::importFilterSettingsXML(QXmlStreamAttributes attributes)
{
    this->radius = attributes.value("radius").toInt();
}

Filter2D* GaussMetaFilter2D::createFilter() const
{
    GaussFilter2D* filter = new GaussFilter2D();
    //filter->setName("Gauss Filter");
    filter->setMetaName(this->objectName());
    return filter;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
