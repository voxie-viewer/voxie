#pragma once

#include <Voxie/data/floatimage.hpp>

#include <Voxie/histogram/histogram.hpp>

#include <QtCore/QRunnable>
#include <QtCore/QVector>

class HistogramWorker :
        public QObject,
        public QRunnable
{
    Q_OBJECT
public:
    HistogramWorker(voxie::data::FloatImage sourceImage, voxie::utilities::Histogram* histogram);
    void run() override;

signals:
    void histogramCalculated(QSharedPointer<QVector<int>> histoData);

private:
    voxie::data::FloatImage sourceImage;
    voxie::utilities::Histogram* histogram;
};



// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
