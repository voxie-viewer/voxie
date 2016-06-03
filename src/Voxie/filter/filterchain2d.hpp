#pragma once

#include <Voxie/data/floatimage.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <Voxie/filter/filter2d.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

namespace voxie{
namespace filter{
/**
 * @brief The FilterChain2D class
 * This class holds a multiple number of filters and manages them.
 * e.g. adding/removing new filter to chain, change position of a filter within the chain,
 * applying all filter within a chain to a image
 */
class VOXIECORESHARED_EXPORT FilterChain2D:
        public QObject
{
    Q_OBJECT

    //private attributes
private:
    QVector<voxie::filter::Filter2D*> filters;
    voxie::data::FloatImage outputFloatImage;
    voxie::data::SliceImage outputSliceImage;

    //public constructor
public:
    FilterChain2D(QObject* parent=nullptr): QObject(parent), outputFloatImage(0,0,false), outputSliceImage() {}
    ~FilterChain2D();

public:
   /**
     * @brief applyTo
     * applies all filters saved in this chain to a FloatImage slice, stores output
     * and sends a signal when its done
     * @param slice FloatImage before filterchain applied
     */
    Q_INVOKABLE void applyTo(voxie::data::FloatImage slice);

    /**
      * @brief applyTo
      * applies all filters saved in this chain to a SliceImage slice, stores output
      * and sends a signal when its done
      * @param slice SliceImage before filterchain applied
      */
    Q_INVOKABLE void applyTo(voxie::data::SliceImage slice);

    /**
     * @brief addFilter
     * adds a new filter to the end of the filterchain
     * @param filter to be added
     */
    Q_INVOKABLE
    void addFilter(voxie::filter::Filter2D* filter);

    /**
     * @brief removeFilter
     * removes this filter from the filterchain
     * @param filter to be removed
     */
    Q_INVOKABLE
    void removeFilter(voxie::filter::Filter2D* filter);

    /**
     * @brief changePosition
     * puts the filter to the specific position in the chain.
     * @param filter this filter changes position
     * @param pos position in the filterchain
     */
    Q_INVOKABLE
    void changePosition(voxie::filter::Filter2D* filter, int pos);


    /**
     * @brief getFilters
     * getter for filterchain
     * @return QVector the filterchain
     */
    Q_INVOKABLE
    QVector<Filter2D *> getFilters();

    /**
     * @brief getFilter
     * getter for a single filter on specified position
     * @param position of the filter to be returned
     * @return  filter
     */
    Q_INVOKABLE
    voxie::filter::Filter2D* getFilter(int pos);

    /**
     * @brief getOutputImage
     * getter for output FloatImage
     * @return FloatImage after all filter of chain applied
     */
    Q_INVOKABLE
    voxie::data::FloatImage &getOutputImage();

    Q_INVOKABLE
    data::SliceImage &getOutputSlice();

    Q_INVOKABLE
    void toXML(QString fileName);

    Q_INVOKABLE
    void fromXML(QString fileName);

public slots:
    void onPlaneChanged(const data::Plane &oldPlane, const data::Plane &newPlane, bool equivalent);

signals:
    void filterChanged(Filter2D* filter);
    void filterListChanged();
    void allFiltersApplied();

};

} //filter
} //voxie

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
