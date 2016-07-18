#pragma once

#include <Voxie/histogram/histogram.hpp>

#include <QtCore/QVector>

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

class HistogramWidget : public QWidget
{
    Q_OBJECT

    //private attributes
private:
    QWidget *paintArea = nullptr;
    voxie::utilities::Histogram *histogram = nullptr;
    QVector<int> histoData;
    QDoubleSpinBox *spinUpperBound = nullptr;
    QDoubleSpinBox *spinLowerBound = nullptr;
    QSpinBox *spinMaxYValue = nullptr;
    QCheckBox *maxYValueCheckBox = nullptr;
    QDialog *dialog = nullptr;
    QCheckBox *logCheckBox = nullptr;
    int maxYVal = 150;
    int yCoordMaxLabel; //y coordinate for labeling highest value on y-Axis
    int yCoordMaxLabelLog; //y coordinate for labeling highest value on y-Axis in log view
    //public constructor
public:
    HistogramWidget(QWidget *parent = 0);

    //public methods
public:
    void setHistogramData(const QVector<int>&);

    //private methods
private:
    /**
     * @brief drawHistogram
     * @param painter
     * Initiates the correct drawing (standard or log)
     */
    void drawHistogram(QPainter *painter);

    /**
     * @brief drawStandardHistogram
     * @param painter
     * Draws a normal histogram with values from histogram
     */
    void drawStandardHistogram(QPainter *painter);

    /**
     * @brief drawLogHistogram
     * @param painter
     * Draws a histogram where all values are in logarithm.
     */
    void drawLogHistogram(QPainter *painter);

    /**
     * @brief drawAxes
     * @param painter
     * Draws the x and y axis
     */
    void drawAxes(QPainter *painter);

    /**
     * @brief drawAxesLabel
     * @param painter
     * Draws the labeling for a x and y axis.
     * On y axis the rate of values
     * On x axis range from upperBound to loweBound of histogram
     */
    void drawAxesLabel(QPainter *painter);

    /**
     * @brief getHighestValue
     * @return the highest value in histogram
     * Returns the highest value in histogram
     */
    int getHighestValue();
    void resizeEvent(QResizeEvent *ev);

signals:
    /**
     * @brief histogramSettingsChanged
     * signal is send when any settings of the widget are
     * changed to initiate calculating new histogram
     */
    void histogramSettingsChanged();

public slots:
    /**
     * @brief calculateHistogram
     * @param image contains data for the histogram
     * Start a new worker to generate histogram  threaded
     */
    void calculateHistogram(voxie::data::FloatImage image);

protected slots:
    void paintEvent(QPaintEvent* ev);

    /**
     * @brief openSettingsDialog
     * creates or opens dialog for changing upper/lower bound and max value limit
     */
    void openSettingsDialog();

    /**
     * @brief updateSettings
     * updates histogram datastructure from data in settings
     */
    void updateSettings();

    /**
     * @brief histogramCalculated
     * @param histoData new calculated data
     * A new calculated histogram gets stored and drawed
     */
    void histogramCalculated(QSharedPointer<QVector<int>> histoData);

};



// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
