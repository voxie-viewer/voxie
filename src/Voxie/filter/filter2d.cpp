#include "filter2d.hpp"

#include <Voxie/filtermask/imagecomparator.hpp>

using namespace voxie::filter;
using namespace voxie::data;

Filter2D::Filter2D(QObject *parent, Selection2DMask* mask) :
    QObject(parent), mask(mask)
{
    if(mask == nullptr){
        this->mask = new Selection2DMask(this);
    }
    connect(this->mask, &Selection2DMask::changed, this, &Filter2D::triggerFilterChanged);
}

FloatImage Filter2D::applyTo(FloatImage input)
{
    FloatImage output = input.clone();
    this->applyTo(input, output);
    ImageComparator::compareImage(input, output, QRectF(QPointF(0,0), output.getDimension()), this->mask);
    //ImageComparator::compareImageCPU(input, output, QRectF(QPointF(0,0), output.getDimension()), this->mask);
    return output;
}

SliceImage Filter2D::applyTo(SliceImage input)
{
    SliceImage output = input.clone();
    this->applyTo(input, output);
    ImageComparator::compareImage(input, output, output.context().planeArea, this->mask);
    //ImageComparator::compareImageCPU(input, output, output.context().planeArea, this->mask);
    return output;
}

bool Filter2D::isEnabled()
{
    return this->enabled;
}

void Filter2D::setEnabled(bool enable)
{
    if(this->enabled != enable)
    {
        this->enabled = enable;
        emit filterChanged(this);

    }
}

/**void Filter2D::setName(QString name)
{
    this->name = name;
}

QString Filter2D::getName()
{
  return this->name;
}*/


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
