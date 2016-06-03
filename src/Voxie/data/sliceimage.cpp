#include "sliceimage.hpp"

using namespace voxie::data;

float
SliceImage::distanceInMeter(const QPoint &p1, const QPoint &p2) const
{
    // Value for invertYAxis does not matter for distanceInMeter(), use false
	return QLineF( pixelToPlanePoint(p1, false), pixelToPlanePoint(p2, false) ).length();
}

void
SliceImage::imagePoint2PlanePoint(ptrdiff_t x, ptrdiff_t y, const QSize& imgSize, const QRectF& planeArea, QPointF& targetPoint, bool invertYAxis)
{
	qreal relX = x / (imgSize.width() * 1.0);
	qreal relY = y / (imgSize.height() * 1.0);

	targetPoint.setX(relX*planeArea.width() + planeArea.left());
    if (invertYAxis)
        targetPoint.setY(-relY*planeArea.height() + planeArea.bottom());
    else
        targetPoint.setY(relY*planeArea.height() + planeArea.top());
}


QPointF
SliceImage::pixelToPlanePoint(const QPoint &pixelpoint, bool invertYAxis) const
{
	QPointF p;
	imagePoint2PlanePoint(
				pixelpoint.x(), pixelpoint.y(),
				this->getDimension(),
				this->context().planeArea,
				p,
				invertYAxis);
	return p;
}

SliceImage
SliceImage::clone(bool enableSharedMemory) const
{
    SliceImage _clone;
    _clone.imageData->width = this->getWidth();
    _clone.imageData->height = this->getHeight();
    _clone._context = this->_context;

    bool clFailed = false;
    if(this->getMode() == CLMEMORY_MODE){
        try{
            _clone.imageData->clPixels = this->getCLBufferCopy(); // might throw
            _clone.imageData->pixels = this->imageData->pixels.copy(enableSharedMemory); // not the same as getBufferCopy
            _clone.imageData->clInstance = this->imageData->clInstance;
            _clone.imageData->mode = CLMEMORY_MODE;
        } catch(opencl::CLException&){
            clFailed = true;
        }
    }
    if(this->getMode() == STDMEMORY_MODE || clFailed){
        _clone.imageData->pixels = this->getBufferCopy();
    }
    return _clone;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
