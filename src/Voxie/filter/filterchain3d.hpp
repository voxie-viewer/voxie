#pragma once

#include <Voxie/filter/filter3d.hpp>

#include <QtCore/QObject>
#include <QtCore/QVector>

namespace voxie{
// Forward declarations
namespace data{
class DataSet;
class VoxelData;
}

namespace filter{

class VOXIECORESHARED_EXPORT FilterChain3D:
        public QObject
{
    Q_OBJECT

private:
    QVector<voxie::filter::Filter3D*> filters;
    QSharedPointer<voxie::data::VoxelData> outputVolume;
    bool signalOnChange = true;

public:
    FilterChain3D(QObject* parent=nullptr);
    ~FilterChain3D();


public:
    bool signalOnChangeEnabled() {return signalOnChange;}
    void enableSignalOnChange(bool enable){this->signalOnChange = enable;}
    void onFilterChanged(Filter3D* sender){if(this->signalOnChangeEnabled()){emit this->filterChanged(sender);}}

    /**
      * @brief applyTo
      * applies all filters saved in this chain to a volume, stores output
      * and sends a signal when its done
      * @param volume DataSet to apply the filterchain to
      */
    Q_INVOKABLE
    void applyTo(voxie::data::DataSet* dataSet);

    /**
     * @brief addFilter
     * adds a new filter to the end of the filterchain
     * @param filter to be added
     */
    Q_INVOKABLE
    void addFilter(voxie::filter::Filter3D* filter);

    /**
     * @brief removeFilter
     * removes this filter from the filterchain
     * @param filter to be removed
     */
    Q_INVOKABLE
    void removeFilter(voxie::filter::Filter3D* filter);

    /**
     * @brief changePosition
     * puts the filter to the specific position in the chain. Each filter behind this position
     * are put one position to the end of the chain
     * @param filter this filter changes position
     * @param pos position in the filterchain
     */
    Q_INVOKABLE
    void changePosition(voxie::filter::Filter3D* filter, int pos);

    /**
     * @brief getFilters
     * getter for filterchain
     * @return QVector the filterchain
     */
    Q_INVOKABLE
    QVector<Filter3D *> getFilters();

    /**
     * @brief getFilter
     * getter for a single filter on specified position
     * @param position of the filter to be returned
     * @return  filter
     */
    Q_INVOKABLE
    voxie::filter::Filter3D* getFilter(int pos);

    /**
     * @brief getOutputVolume
     * getter for output VoxelData
     * @return VoxelData after all filter of chain applied
     */
    Q_INVOKABLE
    QSharedPointer<voxie::data::VoxelData> getOutputVolume();

    Q_INVOKABLE
    void toXML(QString fileName);

    Q_INVOKABLE
    void fromXML(QString fileName);

signals:
    void filterListChanged();
    void filterChanged(Filter3D* sender);
    void allFiltersApplied();
};


}//filter
}//voxie


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
