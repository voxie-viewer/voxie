#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QVector>

#include <QtGui/QRgb>

namespace voxie {
namespace data {

/**
 * The Colorizer defines a mapping of float values to integer RGBA values
 * for converting FloatImage to QImage. It defines what colors correspond to
 * which float value. Color is interpolated for values that dont have a distinct
 * mapping.
 * @brief The Colorizer class
 */
class VOXIECORESHARED_EXPORT Colorizer : public QObject
{
    Q_OBJECT
public:
    explicit Colorizer(QObject *parent = 0);

    /** removes all mappings */
    void clearMappings()
        {colorMap.clear(); emit this->mappingsCleared();}

    /**
     * inserts a new mapping, if there already existed a mapping for in-value
     * it will be woverwritten. Will emit mappingAdded() or mappingChanged().
     * @param in input value
     * @param out output value
     */
    Q_INVOKABLE void putMapping(float in, QRgb out);

    /**
     * inserts a new mapping, if there already existed a mapping for in-value
     * it will be woverwritten. Will emit mappingAdded() or mappingChanged().
     * @param mapping
     */
    void putMapping(QPair<float,QRgb> mapping)
        {this->putMapping(mapping.first, mapping.second);}

    /**
     * removes a mapping from the colorizer. Will emit mappingRemoved().
     * @param in mapping to remove
     * @return true if mapping existed and was sucessfully removed
     */
    Q_INVOKABLE bool removeMapping(float in);

    /**
     * tests whether a mapping exists for given input value
     * @param in input value to be tested
     * @return true if there exists a mapping fir the value
     */
    Q_INVOKABLE bool hasMapping(float in) const
        {return colorMap.contains(in);}

    /**
     * changes an existing mapping, if there exists no mapping for given value
     * nothing will happen. Will emit mappingChanged() in case a mapping was changed.
     * @param in value for which mapping should be changed
     * @param newOut new output value for in
     */
    Q_INVOKABLE void changeMapping(float in, QRgb newOut);

    /**
     * @param in input value
     * @return mapping for in, when no mapping exists for in then 0 (0,0,0,0) is
     * returned.
     */
    Q_INVOKABLE QRgb getMapping(float in) const;

    /**
     * sets Mapping for NaN value
     * @param color
     */
    Q_INVOKABLE void setNanColor(QRgb color)
        {nanColor = color;}

    /**
     * @return output value for NaN
     */
    Q_INVOKABLE QRgb getNanColor() const
        {return nanColor;}

    /**
     * @return all input values that have mappings in acending order.
     */
    Q_INVOKABLE QVector<float> getInputs() const
        {return colorMap.keys().toVector();}

    /**
     * @return all output values in order of their corresponding input value.
     */
    Q_INVOKABLE QVector<QRgb> getOutputs() const
        {return colorMap.values().toVector();}

    /**
     * @param in input value
     * @return interpolated color for in value
     */
    Q_INVOKABLE QRgb getColor(float in) const;

    /**
     * @return all mappings in ascending order of their input values
     */
    QVector< QPair<float,QRgb> > getMappings() const;

    /**
     * @return number of mappings in colorizer
     */
    Q_INVOKABLE int getNumMappings() const
        {return colorMap.size();}

    /**
     * @return reference to this colorizers color-map.
     */
    const QMap<float,QRgb>& getColorMap()
        {return this->colorMap;}

    /**
     * @return a clone of this Colorizer
     */
    Colorizer* clone() const;

    /**
     * @param ioDevice exports colorizers mappings to ioDevice in XML format.
     * if the ioDevice is already opened in readOnly mode the method cannot
     * export to it. Either pass an unopened device or one that can be written to.
     */
    void toXML(QIODevice* ioDevice) const;

    /**
     * @param filename exports colorizers mappings to given file in XML format.
     */
    Q_INVOKABLE void toXML(QString filename) const;

    /**
     * loads colorizer mappings from xml from given ioDevice. Will emit mappingsCleared()
     * and for each mapping mappingAdded() when sucessfully loaded.
     * @param ioDevice
     */
    void loadFromXML(QIODevice* ioDevice);

    /**
     * loads colorizer mappings from xml from given file. Will emit mappingsCleared()
     * and for each mapping mappingAdded() when sucessfully loaded.
     * @param filename
     */
    Q_INVOKABLE void loadFromXML(QString filename);

public:
    static Colorizer* redPeakColorizer();
    static Colorizer* increasingTransparencyColorizer();
    static Colorizer* fromXML(QIODevice* ioDevice);

private:
    QMap<float,QRgb> colorMap;
    QRgb nanColor;
signals:
    void mappingAdded(QPair<float,QRgb> mapping);
    void mappingRemoved(QPair<float,QRgb> mapping);
    void mappingChanged(float in, QRgb oldOut, QRgb newOut);
    void mappingsCleared();

public slots:

};

}}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
