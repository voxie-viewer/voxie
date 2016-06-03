#include "imagegeneratorworker.hpp"

using namespace voxie::data;

//volatile int ImageGeneratorWorker::started = 0;
//volatile int ImageGeneratorWorker::finished = 0;

ImageGeneratorWorker::ImageGeneratorWorker(QVector<voxie::data::Slice*> slices,
                                           const QRectF &sliceAreaFirst,
                                           const QRectF &sliceAreaSecond,
                                           const QSize &imageSize,
                                           InterpolationMethod interpolation) :
    QObject(),
    slices(slices),
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
    SliceImage result_s1 = slices.at(0)->generateImage(sliceAreaFirst, imageSize, interpolation);
    SliceImage result_s2 = slices.at(1)->generateImage(sliceAreaSecond, imageSize, interpolation);

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
