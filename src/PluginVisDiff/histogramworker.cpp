#include "histogramworker.hpp"

HistogramWorker::HistogramWorker(voxie::data::FloatImage sourceImage, voxie::utilities::Histogram *histogram):
    QObject(),
    sourceImage(sourceImage.clone()),
    histogram(histogram->clone(this))
{
    sourceImage.switchMode(voxie::data::FloatImage::STDMEMORY_MODE);
}

void HistogramWorker::run()
{
    if (thread() != nullptr)
        qCritical() << "Object already has a thread:" << this;
    else
        moveToThread(QThread::currentThread());
    auto data = this->histogram->calculateHistogram(this->sourceImage);
    emit histogramCalculated(data);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
