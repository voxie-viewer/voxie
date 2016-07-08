#include "filter3d.hpp"

using namespace voxie::filter;
using namespace voxie::data;

Filter3D::Filter3D(QObject *parent) :
    QObject(parent), enabled(true), mask(new Selection3DMask(this))
{
    connect(this->mask, &Selection3DMask::changed, this, &Filter3D::triggerFilterChanged);
}

QSharedPointer<VoxelData> Filter3D::applyTo(const QSharedPointer<VoxelData>& input)
{
    QSharedPointer<VoxelData> source = this->getSourceVolume(input);
    this->applyTo(source, input);
    input->invalidate();
    return input;
}

bool Filter3D::isEnabled()
{
    return this->enabled;
}

void Filter3D::setEnabled(bool enable)
{
    if(this->enabled != enable)
    {
        this->enabled = enable;
        emit filterChanged(this);
    }
}

/**void Filter3D::setName(QString name)
{
    this->name = name;
}

QString Filter3D::getName()
{
  return this->name;
}*/

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
