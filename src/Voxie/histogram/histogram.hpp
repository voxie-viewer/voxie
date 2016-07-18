#pragma once

#include <Voxie/data/floatimage.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

namespace voxie{
namespace utilities{

/**
 * @brief The Histogram class
 * This class generates a histogram.
 * Values that are counted can be limited and put into classes
 *
 */
class VOXIECORESHARED_EXPORT Histogram : public QObject
{
    Q_OBJECT

    //private attributes
private:
   float upperBound, lowerBound;
    int resolution;

    //public constructor
public:
    Histogram(QObject *parent = 0,float lowerBound =0,float upperBound=0,int resolution=0);
    ~Histogram();
    //public methods
public:
    /**
     * @brief getUpperBound
     * getter for upperBound
     * @return upperBound
     */
    Q_INVOKABLE
    float getUpperBound();

    /**
     * @brief setUpperBound
     * setter for upperBound
     * @param upperBound
     */
    Q_INVOKABLE
    void setUpperBound(float upperBound);

    /**
     * @brief getLowerBound
     * getter for lowerBound
     * @return lowerBound
     */
    Q_INVOKABLE
    float getLowerBound();

    /**
     * @brief setLowerBound
     * setter for lowerBound
     * @param lowerBound
     */
    Q_INVOKABLE
    void setLowerBound(float lowerBound);

    /**
     * @brief getResolution
     * getter for resolution
     * @return resolution
     */
    Q_INVOKABLE
    int getResolution();

    /**
     * @brief setResolution
     * setter for resolution
     * @param resolution
     */
    Q_INVOKABLE
    void setResolution(int resolution);

    /**
     * @brief calculateHistogram
     * Counts pixelvalues in a specific range (limited with upper/lowerBound)
     * and stores it in the QVector.
     * Pixelvalues are put into classes specified by resolution.
     * E.g. lowerBound=0.0; upperBound=1.0; resolution 5
     * that means that the histogram has 5 bars and each bar stores values in a range of 0.2
     * (first bar 0.0-0.2 second bar 0.3-0.4 .....)
     * @param img pixelvalues of this FloatImage are counted
     */
    Q_INVOKABLE
    QSharedPointer<QVector<int>> calculateHistogram(data::FloatImage img);

    Histogram* clone(QObject* parent = 0) const;

signals:
    /**
    * @brief histogramReady
    * signal is send when histogram is calculated
    * @param histogram
    */
   void histogramReady(QVector<int> histogram);


};

} //utilities
} //voxie

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
