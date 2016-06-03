#pragma once

#include <Voxie/data/floatimage.hpp>

#include <Voxie/filtermask/ellipseData.hpp>
#include <Voxie/filtermask/polygonData.hpp>
#include <Voxie/filtermask/rectangleData.hpp>
#include <Voxie/filtermask/selection2dmask.hpp>

#include <Voxie/opencl/clinstance.hpp>

namespace voxie {
namespace filter {
/**
 * This class is used by applying a filter and also applying a mask on it. Therefor are 2 methods, compareImage.
 * This method works on GPU only. But it has a fallback methods, when the GPU code won't works for some problems like,
 * there is no GPU or the memory is not enough.
 * @brief The ImageComparator class is a class with the static methods for applying the masks of a Filter.
 * The methods are for GPU and a fallback method for CPU.
 */
class ImageComparator
{
public:
    /**
     * @brief compareImage compares if a point is inside the mask and writes the result on the filterImage. Works on GPU
     * @param sourceImage the normal image without an applied filter.
     * @param filterImage the image which a filter is applied on. Contains the result in the end.
     * @param area the area where the picture is on the plane.
     * @param mask the masks of the given filter.
     */
    static void compareImage(voxie::data::FloatImage sourceImage, voxie::data::FloatImage filterImage, QRectF area, Selection2DMask* mask);

    /**
     * @brief compareImageCPU do the same as compareImage, but it is a fallback when GPU won't work or do a failure.
     * @param sourceImage the normal image without an applied filter.
     * @param filterImage the image which a filter is applied on. Contains the result in the end.
     * @param area the area where the picture is on the plane.
     * @param mask the masks of the given filter.
     */
    static void compareImageCPU(voxie::data::FloatImage sourceImage, voxie::data::FloatImage filterImage, QRectF area, Selection2DMask* mask);
};
}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
