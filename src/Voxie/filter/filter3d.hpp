#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/filtermask/selection3dmask.hpp>

#include <QtCore/QObject>
#include <QtCore/QXmlStreamAttributes>

#include <QtWidgets/QDialog>

namespace voxie{
namespace filter{

/**
 * @brief The Filter3D class
 * This class implements already most of the methods.
 * Concrete filter inheriting from this class, need to implement
 * methods getTargetVolume and applyTo().
 *
 */
class VOXIECORESHARED_EXPORT Filter3D :
        public QObject
{

    Q_OBJECT

    //private attributes
private:
    bool enabled;
    QString metaName;
    Selection3DMask* mask;

    //public constructor
public:
    Filter3D(QObject* parent=nullptr);


public:

    /**
     * @brief applyTo
     * @param input
     * @return
     */
    Q_INVOKABLE
    virtual voxie::data::VoxelData* applyTo(voxie::data::VoxelData* input);


    /**
     * @brief isEnabled
     * Method to check wether the filter is enabled or disabled
     * @return true if filter is enabled, flase if disabled
     */
    virtual bool isEnabled();


    /**
     * @brief setEnabled
     * Setter for the enabled attribute
     * @param enable true if filter shall enabled else false
     */
    Q_INVOKABLE
    virtual void setEnabled(bool enable);

    /**
    * @brief setMetaName
    * Setter for filter name
    * @param metaName name to set
    */
    Q_INVOKABLE
    void setMetaName(QString metaName)
    {
        this->metaName = metaName;
    }

    /**
     * @brief getMetaName
     * Getter for filter name
     * @return filtername
     */
    Q_INVOKABLE
    QString getMetaName()
    {
        return this->metaName;
    }

    /**
     * @brief getSettingsDialog
     * A pointer to a customaziable QDialog that allows to modify filter settings.
     */
    Q_INVOKABLE
    virtual QDialog* getSettingsDialog() {
        return nullptr;
    }


    /**
     * @brief hasSettingsDialog
     * returns whether the filter has a settings dialog or not (default true).
     */
    virtual bool hasSettingsDialog() {
        return false;
    }

    /**
     * @return this filter's mask
     */
    Q_INVOKABLE
    Selection3DMask* getMask() {
        return this->mask;
    }

    /**
     * @brief exportFilterSettingsXML
     * Exports all settings of a concrete filter.
     * @return
     */
    Q_INVOKABLE
    virtual QXmlStreamAttributes exportFilterSettingsXML() = 0;

    /**
     * @brief importFilterSettingsXML
     * Imports all settings of a filter from xml
     * @param attributes
     */
    Q_INVOKABLE
    virtual void importFilterSettingsXML(QXmlStreamAttributes attributes) = 0;

protected:

    /**
    * @brief getSourceVolume
    * returns a volume to be used as source when aplying
    * the filter on input. The returned volume can be input volume,
    * when filter can work in-place
    * @param input volume for which a source volume should be returned
    * @return source volume for filter result
    */
    virtual voxie::data::VoxelData* getSourceVolume(voxie::data::VoxelData* input)=0;

    /**
     * @brief applyTo
     * Applies this filter to input volume and stores result in output.
     * This method must be implemented by a concrete filter
     * @param input volume before filter applied
     * @param output volume after filter applied
     */
    virtual void applyTo(voxie::data::VoxelData* source, voxie::data::VoxelData* target)=0;

    /**
     * convenience method for 'emit this->filterchanged(this)'
     */
    void triggerFilterChanged(){emit this->filterChanged(this);}

signals:

    /**
     * @brief filterChanged
     * This signal is send when any of the filters data has changed
     * e.g. enabled/disabled, filter's mask or concrete filter settings changed.
     */
    void filterChanged(Filter3D* sender);
};

} //namespace filter
} //namspace voxie

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
