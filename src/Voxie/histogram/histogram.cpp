#include "histogram.hpp"

#include <Voxie/scripting/scriptingcontainer.hpp>

using namespace voxie::utilities;

Q_DECLARE_METATYPE(QSharedPointer<QVector<int> >)

Histogram::Histogram(QObject *parent, float lowerBound, float upperBound, int resolution) :
    QObject(parent), upperBound(upperBound), lowerBound(lowerBound),  resolution(resolution)
{
    qRegisterMetaType<QSharedPointer<QVector<int> > >();
}

Histogram::~Histogram()
{
}

QSharedPointer<QVector<int>> Histogram::calculateHistogram(voxie::data::FloatImage img)
{
    if (thread() != QThread::currentThread())
        qCritical() << "Histogram::calculateHistogram() called from wrong thread" << QThread::currentThread() << "(belongs to" << thread() << ")";

    auto histo = createQSharedPointer<QVector<int>>(resolution, 0);
	float range = (this->upperBound - this->lowerBound);
    float lowerBound = this->lowerBound;
    //for every bar in histogram go through img and check if pixelvalue is in range
    FloatBuffer buffer = img.getBufferCopy();
    for(size_t i= 0; i < buffer.length(); i++)
    {
        float pixel_value = buffer[i];
        int slot = static_cast<int>((histo->length() - 1)*((pixel_value - lowerBound) / range));
        if((slot < 0) || (slot >= histo->length()))
            continue;
        (*histo)[slot]++;
    }

    return histo;
}

float Histogram::getUpperBound()
{
	return this->upperBound;
}

void Histogram::setUpperBound(float upperBound)
{
	this->upperBound = upperBound;
}

float Histogram::getLowerBound()
{
	return this->lowerBound;
}

void Histogram::setLowerBound(float lowerBound)
{
	this->lowerBound = lowerBound;
}

int Histogram::getResolution()
{
	return this->resolution;
}

void Histogram::setResolution(int resolution)
{
	this->resolution = resolution;
}

Histogram* Histogram::clone(QObject* parent) const {
    return new Histogram(parent, lowerBound, upperBound, resolution);
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
