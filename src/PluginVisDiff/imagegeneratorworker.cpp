#include "imagegeneratorworker.hpp"

using namespace voxie::data;

//volatile int ImageGeneratorWorker::started = 0;
//volatile int ImageGeneratorWorker::finished = 0;

ImageGeneratorWorker::ImageGeneratorWorker(const QSharedPointer<VoxelData>& data1,
                                           const QSharedPointer<VoxelData>& data2,
                                           const voxie::data::Plane& plane1,
                                           const voxie::data::Plane& plane2,
                                           const QRectF &sliceAreaFirst,
                                           const QRectF &sliceAreaSecond,
                                           const QSize &imageSize,
                                           InterpolationMethod interpolation) :
    QObject(),
    data1(data1),
    data2(data2),
    plane1(plane1),
    plane2(plane2),
    sliceAreaFirst(sliceAreaFirst),
    sliceAreaSecond(sliceAreaSecond),
    imageSize(imageSize),
    interpolation(interpolation)
{
}

ImageGeneratorWorker::~ImageGeneratorWorker()
{
}

void ImageGeneratorWorker::run() {
    SliceImage result_s1 = Slice::generateImage(data1.data(), plane1, sliceAreaFirst, imageSize, interpolation);
    SliceImage result_s2 = Slice::generateImage(data2.data(), plane2, sliceAreaSecond, imageSize, interpolation);

    // generate diff image
    FloatImage diff_image(result_s1.getWidth(), result_s1.getHeight(), false);

    for(size_t w = 0; w < result_s1.getWidth(); ++w)
    {
        for(size_t h = 0; h < result_s1.getHeight(); ++h)
        {
            diff_image.setPixel(w,h, std::abs(result_s1.getPixel(w,h) - result_s2.getPixel(w,h)));
        }
    }

    DiffSliceImage *dsi = new DiffSliceImage();
    dsi->first = result_s1;
    dsi->second = result_s2;
    dsi->diffImage = diff_image;

    emit imageGenerated(dsi);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
