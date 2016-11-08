#include "boxblur3d.hpp"

#include <memory>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

BoxBlur3D::BoxBlur3D(QObject *parent) :
    Filter3D(parent),
    dialog(nullptr),
    radius(1)
{
}


/* returns a if a not nan, else returns default */
#define IGNORENAN(a, _default) ( std::isnan((a)) ? (_default):(a) )
/* returns a if a not nan, else returns 0 */
#define IGNORENAN0(a) (IGNORENAN((a),0) )

void
BoxBlur3D::applyTo(const QSharedPointer<voxie::data::VoxelData>& input, const QSharedPointer<voxie::data::VoxelData>& output)
{
    if (input != output) {
        qCritical() << "BoxBlur3D::applyTo: input != output";
        return;
    }
    if(this->radius < 1){
            return;
    }

    voxie::scripting::IntVector3 coord(0,0,0);
    float totalValue = 0;
    size_t numValues = this->radius*2 + 1;
    std::unique_ptr<float[]> values(new float[numValues]);
    const voxie::scripting::IntVector3 size = input->getDimensions();
    const QVector3D gridSpacing = input->getSpacing();
    const QVector3D volOrig = input->getOrigin();

    // in x-direction
    for(size_t y = 0; y < size.y; y++){
        for(size_t z = 0; z < size.z; z++){
            totalValue = 0;
            // setUpValues for first in row
            float leftVal = input->getVoxel(0,y,z);
            float rightVal = input->getVoxel(size.x-1,y,z);
            for(int i = -radius; i <= radius; i++){
                if(i < 0){
                    totalValue += IGNORENAN0(leftVal);
                    values[i+radius] = leftVal;
                } else if(i >= (int) size.x){
                    values[i+radius] = rightVal;
                    totalValue += IGNORENAN0(values[i+radius]);
                } else {
                    values[i+radius] = input->getVoxel(i,y,z);
                    totalValue += IGNORENAN0(values[i+radius]);
                }
            }
            if(this->getMask()->contains(0,y,z, gridSpacing, volOrig))
                input->setVoxel(0,y,z, totalValue/numValues);

            coord.z = z;
            coord.y = y;
            // iterate over x-row
            for(size_t x = 1; x < size.x; x++){
                coord.x = x;

                totalValue -= IGNORENAN0(values[x%numValues]);
                int i = x + radius;
                if(i >= (int)size.x){
                    values[x%numValues] = rightVal;
                    totalValue += IGNORENAN0(values[x%numValues]);
                } else {
                    values[x%numValues] = input->getVoxel(i,y,z);
                    totalValue += IGNORENAN0(values[x%numValues]);
                }
                if(this->getMask()->contains(coord, gridSpacing, volOrig))
                    input->setVoxel(coord, totalValue/numValues);
            }
        }
    }

    // in y-direction
    for(size_t z = 0; z < size.z; z++){
        for(size_t x = 0; x < size.x; x++){
            totalValue = 0;
            // setUpValues for first in row
            float leftVal = input->getVoxel(x,0,z);
            float rightVal = input->getVoxel(x,size.y-1,z);
            for(int i = -radius; i <= radius; i++){
                if(i < 0){
                    totalValue += IGNORENAN0(leftVal);
                    values[i+radius] = leftVal;
                } else if(i >= (int)size.y){
                    values[i+radius] = rightVal;
                    totalValue += IGNORENAN0(values[i+radius]);
                } else {
                    values[i+radius] = input->getVoxel(x,i,z);
                    totalValue += IGNORENAN0(values[i+radius]);
                }
            }
            if(this->getMask()->contains(x,0,z, gridSpacing, volOrig))
                input->setVoxel(x,0,z, totalValue/numValues);

            coord.z = z;
            coord.x = x;
            // iterate over y-row
            for(size_t y = 1; y < size.y; y++){
                coord.y = y;

                totalValue -= IGNORENAN0(values[y%numValues]);
                int i = y + radius;
                if(i >= (int)size.y){
                    values[y%numValues] = rightVal;
                    totalValue += IGNORENAN0(values[y%numValues]);
                } else {
                    values[y%numValues] = input->getVoxel(x,i,z);
                    totalValue += IGNORENAN0(values[y%numValues]);
                }
                if(this->getMask()->contains(coord, gridSpacing, volOrig))
                    input->setVoxel(coord, totalValue/numValues);
            }
        }
    }

    // in z-direction
    for(size_t y = 0; y < size.y; y++){
        for(size_t x = 0; x < size.x; x++){
            totalValue = 0;
            // setUpValues for first in row
            float leftVal = input->getVoxel(x,y,0);
            float rightVal = input->getVoxel(x,y,size.z-1);
            for(int i = -radius; i <= radius; i++){
                if(i < 0){
                    totalValue += IGNORENAN0(leftVal);
                    values[i+radius] = leftVal;
                } else if(i >= (int)size.y){
                    values[i+radius] = rightVal;
                    totalValue += IGNORENAN0(values[i+radius]);
                } else {
                    values[i+radius] = input->getVoxel(x,y,i);
                    totalValue += IGNORENAN0(values[i+radius]);
                }
            }
            if(this->getMask()->contains(x,y,0, gridSpacing, volOrig))
                input->setVoxel(x,y,0, totalValue/numValues);

            coord.y = y;
            coord.x = x;
            // iterate over z-row
            for(size_t z = 1; z < size.z; z++){
                coord.z = z;

                totalValue -= IGNORENAN0(values[z%numValues]);
                int i = z + radius;
                if(i >= (int)size.z){
                    values[z%numValues] = rightVal;
                    totalValue += IGNORENAN0(values[z%numValues]);
                } else {
                    values[z%numValues] = input->getVoxel(x,y,i);
                    totalValue += IGNORENAN0(values[z%numValues]);
                }
                if(this->getMask()->contains(coord, gridSpacing, volOrig))
                    input->setVoxel(coord, totalValue/numValues);
            }
        }
    }
}


void
BoxBlur3D::setRadius(int radius)
{
    if(radius != this->radius){
        this->radius = radius;
        this->triggerFilterChanged();
    }
}


QDialog*
BoxBlur3D::getSettingsDialog()
{
    if(this->dialog == nullptr){
        this->dialog = new QDialog;
        this->dialog->setWindowTitle(this->objectName());
        QPushButton* okbtn = new QPushButton("ok");
        QSpinBox* spinRadius = new QSpinBox();
        spinRadius->setMinimum(0);
        spinRadius->setMaximum(100);
        spinRadius->setValue(this->getRadius());
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel("Radius"));
        row->addWidget(spinRadius);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addLayout(row);
        layout->addWidget(okbtn);
        this->dialog->setLayout(layout);

        connect(okbtn, &QPushButton::clicked, this, [=]()
        {this->setRadius(spinRadius->value());});
        connect(this, &voxie::filter::Filter3D::filterChanged, spinRadius, [=]()
        {spinRadius->setValue(this->getRadius() < 0 ? 0: this->getRadius());});
    }
    return this->dialog;
}


QXmlStreamAttributes
BoxBlur3D::exportFilterSettingsXML()
{
    QXmlStreamAttributes attributes;
    attributes.append("radius", QString::number(this->radius));
    return attributes;
}


void
BoxBlur3D::importFilterSettingsXML(QXmlStreamAttributes attributes)
{
    bool valid;
    int radius = attributes.value("radius").toInt(&valid);
    if(valid)
        this->radius = radius;
}


voxie::filter::Filter3D*
BoxBlur3DMeta::createFilter() const
{
    voxie::filter::Filter3D* filter = new BoxBlur3D();
    filter->setMetaName(this->objectName());
    return filter;
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
