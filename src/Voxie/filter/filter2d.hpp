#pragma once

#include <Voxie/data/floatimage.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <Voxie/filtermask/selection2dmask.hpp>

#include <QtCore/QObject>
#include <QtCore/QXmlStreamAttributes>

#include <QtWidgets/QDialog>

namespace voxie{
namespace filter{

/**
 * @brief The Filter2D class
 * This class implements already most of the methods.
 * Concrete filter inheriting from this class, need to implement
 * methods getTargetVolume and applyTo().
 *
 */
class VOXIECORESHARED_EXPORT Filter2D :
        public QObject
{
    //QTAnotataions
    Q_OBJECT

    //private attributes
private:
    bool enabled = true;
    QString metaName;
    voxie::filter::Selection2DMask* mask;

    //public constructor
public:
    Filter2D(QObject* parent=nullptr, Selection2DMask* mask=nullptr);

   //public methods
public:

    /**
     * @brief applyTo
     * Already implemented apply method.
     * This method applies a concrete filter implementation to a FloatImage
     * @param input FloatImage where filter will be applied.
     * @return output FloatImage after filter applied
     */
   Q_INVOKABLE virtual voxie::data::FloatImage applyTo(voxie::data::FloatImage input);


    /**
     * @brief applyTo
     * Already implemented apply method.
     * This method applies a concrete filter implementation to a SliceImage
     * @param input SliceImage where filter will be applied.
     * @return output SliceImage after filter applied
     */
   Q_INVOKABLE virtual voxie::data::SliceImage applyTo(voxie::data::SliceImage input);


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
        return true;
    }

    /**
     * @return this filter's mask
     */
    Q_INVOKABLE
    Selection2DMask* getMask() {
        return this->mask;
    }

    /**
     * @return this filter's mask
     */
    const Selection2DMask* getMask() const {
        return this->mask;
    }

protected:
    /**
     * @brief applyTo
     * Applies this filter to input FloatImage and stores result in output.
     * This method must be implemented by a concrete filter
     * @param input FloatImage before filter applied
     * @param output FloatImage after filter applied
     */
    virtual void applyTo(voxie::data::FloatImage input, voxie::data::FloatImage output)=0;

    /**
     * @brief applyTo
     * Applies this filter to input SliceImage and stores result in output.
     * This method must be implemented by a concrete filter
     * @param input SliceImage before filter applied
     * @param output SliceImage after filter applied
     */
    virtual void applyTo(voxie::data::SliceImage input, voxie::data::SliceImage output)=0;

    /**
     * convenience method for 'emit this->filterchanged(this)'
     */
    void triggerFilterChanged(){emit this->filterChanged(this);}

signals:
    /**
     * will be emitted whenever the filter is enabled or disabled,
     * when the filters filter mask changes or when the filters internal
     * settings are changed
     * @param filter that emmited the signal
     */
    void filterChanged(Filter2D* filter);


};

} //namespace filter
} //namespace voxie

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
