#include "histogramworker.hpp"

HistogramWorker::HistogramWorker(voxie::data::FloatImage sourceImage, voxie::utilities::Histogram *histogram):
    QObject(),
    sourceImage(sourceImage.clone()),
    histogram(histogram)
{
    sourceImage.switchMode(voxie::data::FloatImage::STDMEMORY_MODE);
}

void HistogramWorker::run()
{
    this->histogram->calculateHistogram(this->sourceImage);
    emit histogramCalculated(histogram->getData());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
