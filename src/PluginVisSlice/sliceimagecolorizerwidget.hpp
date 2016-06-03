#pragma once

#include <Voxie/data/colorizer.hpp>
#include <Voxie/data/floatimage.hpp>

#include <QtGui/QImage>

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

/**
 * @brief The SliceImageColorizerWidget class provides an ui wrapper around a colorizer. It can add, change and remove color mappings. It also displays this information in a section.
 * @author Hans Martin Berner
 */
class SliceImageColorizerWidget : public QWidget
{
    Q_OBJECT
public:
    SliceImageColorizerWidget(QWidget *parent = 0);
    ~SliceImageColorizerWidget();
private:
    voxie::data::Colorizer colorizerInstance;
	//QObject* filter;
    QPushButton* addGradientButton;
    QColorDialog* colorPicker;
    QVBoxLayout *layout;
    QPushButton* colorWidget;
    QColor currentColor;
    QLineEdit* currentValueInput;
    QListWidget* box;
	QHBoxLayout* topRow;
	QFrame* line;
    float currentValue;
	bool requestColorizer;
public slots:
	/**
	 * @brief doColorizer runs the colorizer with its current settings on the given image
	 * @param image to be colorized
	 */
    void doColorizer(voxie::data::FloatImage image);
	/**
	 * @brief addMapping adds a mapping to the colorizer. If the in value is present it is overridden
	 * @param in float value used as input
	 * @param out value to be used as output color
	 */
    void addMapping(float in, QRgb out);
	/**
	 * @brief updateButtons rebuilds the section ui
	 */
    void updateButtons();
	/**
	 * @brief updateMapping edits an existing mapping with new values
	 * @param old
	 * @param in
	 * @param out
	 */
    void updateMapping(float old, float in, QRgb out);
	/**
	 * @brief removeMapping deletes the mapping associated with the given in value
	 * @param in
	 */
    void removeMapping(float in);
signals:
	/**
	 * @brief imageColorized signals that an image has been colorized with the doColorizer method
	 * @param image resulting image
	 */
    void imageColorized(QImage image);
	/**
	 * @brief gradientChanged signals that a mapping has changed
	 * @param mapping the resulting mapping after the change
	 */
    void gradientChanged(QMap<float, QRgb> mapping);
	/**
	 * @brief deleteOldButtons used internally to signify that the ui needs to be refreshed and all old buttons should be removed.
	 */
    void deleteOldButtons();
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
