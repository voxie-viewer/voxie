#pragma once

#include <Voxie/data/floatimage.hpp>
#include <Voxie/data/sliceimagecontext.hpp>

#include <QtCore/QLineF>
#include <QtCore/QRectF>
#include <QtCore/QSize>

namespace voxie {
namespace data {

/**
 * SliceImage is a FloatImage that was created from a Slice, appart from the
 * imagedate it stores a SliceImageContext that holds information about the
 * details from what this image was created.
 */
class VOXIECORESHARED_EXPORT SliceImage: public FloatImage
{
public:
    /**
     * @brief SliceImage
     * @param width
     * @param height
     * @param context
     */
    SliceImage(size_t width, size_t height, SliceImageContext context, bool enableSharedMemory) : FloatImage(width, height, enableSharedMemory), _context(context)
    {}

    /**
     * @brief SliceImage constructs an empty sliceimage
     */
    SliceImage() : FloatImage(), _context(SliceImageContext())
    {}

    /**
     * constructs a sliceimage from a floatimage, the slicimagecontext
     * is assumed to have a standard plane, a planearea starting at (0,0) and
     * extending to (imageWidth, imageHeight) (in meters), and a voxelSpacing of
     * (1,1,1) (cubic meter). The Floatimages Data is cloned.
     * @param floatimg for creating sliceimage from
     */
    explicit SliceImage(const FloatImage& floatimg, bool enableSharedMemory) : FloatImage(floatimg.clone(enableSharedMemory)), _context(SliceImageContext())
    {this->_context.planeArea = QRectF(QPointF(0,0),this->getDimension());this->_context.voxelGridSpacing = QVector3D(1,1,1);}

    /**
     * @return the distance of 2 points on the image in meters
     * @param p1 point one
     * @param p2 point two
     */
    float distanceInMeter(const QPoint& p1, const QPoint& p2) const;

    /**
     * @return the point on the plane this image was created on corresponding
     * to a pixel point in the image.
     * @param pixelpoint point on image
     */
    QPointF pixelToPlanePoint(const QPoint& pixelpoint, bool invertYAxis) const;

    /**
     * @return image's context
     */
    const SliceImageContext& context() const
        {return this->_context;}

    static void imagePoint2PlanePoint(ptrdiff_t x, ptrdiff_t y, const QSize& imgSize, const QRectF& planeArea, QPointF& targetPoint, bool invertYAxis);

    /**
     * @return clone of this SliceImage
     */
    SliceImage clone(bool enableSharedMemory = false) const;


private:
    SliceImageContext _context;
};

}}

Q_DECLARE_METATYPE(voxie::data::SliceImage)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
