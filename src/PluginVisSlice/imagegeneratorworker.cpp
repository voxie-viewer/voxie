#include "imagegeneratorworker.hpp"

#include <QtCore/QDebug>

using namespace voxie::data;

//volatile int ImageGeneratorWorker::started = 0;
//volatile int ImageGeneratorWorker::finished = 0;

ImageGeneratorWorker::ImageGeneratorWorker(voxie::data::Slice* slice,
										   const QRectF &sliceArea,
								           const QSize &imageSize,
								           InterpolationMethod interpolation) :
    QObject(),
    slice(slice),
	sliceArea(sliceArea),
	imageSize(imageSize),
    interpolation(interpolation)
{
}

ImageGeneratorWorker::~ImageGeneratorWorker()
{
}

void ImageGeneratorWorker::run() {
    //qDebug() << "started:" << ++ImageGeneratorWorker::started;
    SliceImage result = slice->generateImage(sliceArea, imageSize, interpolation);
    //deprecated//SliceImage* result = SliceImageGenerator::instance().generateImage(&slice, sliceArea, imageSize, interpolation);
    //qDebug() << "finished:" << ++ImageGeneratorWorker::finished;
    emit imageGenerated(result);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
