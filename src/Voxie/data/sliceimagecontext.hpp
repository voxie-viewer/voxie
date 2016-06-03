#pragma once

#include <Voxie/data/plane.hpp>

#include <QtCore/QRectF>

#include <QtGui/QVector3D>

namespace voxie {
namespace data {

/**
 * SliceimageContext stores information about the circumstances in which a
 * sliceimage was created. It stores the Plane it was of the slice it was created
 * from the planeArea that it displays and the voxelGridSpacing of the dataset
 * the pixels were extracted from.
 * @brief The SliceImageContext struct
 */
struct SliceImageContext
{

    Plane cuttingPlane;
    QRectF planeArea;
    QVector3D voxelGridSpacing;

};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
