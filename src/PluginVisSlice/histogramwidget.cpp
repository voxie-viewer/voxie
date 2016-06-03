#include "histogramwidget.hpp"

#include <PluginVisSlice/histogramworker.hpp>

#include <cmath>

#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QThreadPool>

#include <QtGui/QFont>

#include <QtWidgets/QAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::utilities;
using namespace voxie::data;

HistogramWidget::HistogramWidget(QWidget *parent) :
	QWidget(parent)
{
	QString name = "Histogram";
    this->setWindowTitle(name);
    QVBoxLayout *topLayout = new QVBoxLayout(this);

    //--- the paint area for drawing histogram ---
    this->paintArea = new QWidget(this);
    this->paintArea->setMinimumHeight(151);


    //--- Toolbar with settings-bbutton , enable/diable log-view etc. ---
    QToolBar *toolbar = new QToolBar(this);

    //--- Settings Button---
    QAction *settingsButton = toolbar->addAction(QIcon(":/icons/gear.png"),"Settings");
    connect(settingsButton, &QAction::triggered, this, &HistogramWidget::openSettingsDialog);

    //--- log check Box ---
    this->logCheckBox = new QCheckBox("log");
    connect(logCheckBox, &QCheckBox::stateChanged, this, [=](){
        emit this->histogramSettingsChanged();
    });
    toolbar->addWidget(logCheckBox);

    //--- put all together ---
    topLayout->addWidget(this->paintArea);
    topLayout->addWidget(toolbar);

    //--- create Histogram ---
    this->histogram = new Histogram(this, 0,1,this->paintArea->width());

    //checkBox for settings dialog is created here so that program doesnt crash
    //when drawing histogram and settings dialog is not created yet.
    this->maxYValueCheckBox = new QCheckBox("automatic maximum value");
    this->maxYValueCheckBox->setCheckState(Qt::Checked);

    // Make sure maxYValueCheckBox is destroyed even if no settings dialog was created
    QPointer<QCheckBox> maxYValueCheckBoxPtr(maxYValueCheckBox);
    connect(this, &QObject::destroyed, [maxYValueCheckBoxPtr] {
            if (maxYValueCheckBoxPtr)
                delete maxYValueCheckBoxPtr;
        });
}


void HistogramWidget::paintEvent(QPaintEvent* ev)
{
    Q_UNUSED(ev)
    QPainter painter(this);
    drawHistogram(&painter);
    drawAxes(&painter);
    drawAxesLabel(&painter);

}

void HistogramWidget::drawHistogram(QPainter *painter)
{
    if(this->histoData.length() != 0)
    {
        if(this->logCheckBox->checkState() == Qt::Unchecked)
        {
            drawStandardHistogram(painter);
        }
        if(this->logCheckBox->checkState() == Qt::Checked)
        {
            drawLogHistogram(painter);
        }
    }
}

void HistogramWidget::drawStandardHistogram(QPainter *painter)
{

    // data for drawing
    int maxValue = getHighestValue();
    if(this->maxYValueCheckBox->checkState() == Qt::Unchecked)
    {
        maxValue = this->maxYVal;
    }
    float barWidth = (this->paintArea->width()/ (this->histoData.length()*1.0f));

    //draw histogram
    for (int i = 0; i < this->histoData.length(); i++)
    {
        int scaled = this->histoData.at(i) * 150/maxValue; //scale values to range 10-150
        int rectX = (barWidth*i)+14;
        int rectY = this->paintArea->height()-scaled;
        int rectHeight = this->paintArea->height()-rectY;
        painter->fillRect(rectX, rectY,barWidth, rectHeight-1, Qt::red);

        // save highest y- coordinate for labeling later
        if(this->histoData.at(i) == maxValue && (this->maxYValueCheckBox->checkState()==Qt::Checked))
        {
            this->yCoordMaxLabel =  rectY;
        }else if(this->maxYValueCheckBox->checkState() == Qt::Unchecked)
        {
            this->yCoordMaxLabel = 0;
        }
    }

}

void HistogramWidget::drawLogHistogram(QPainter *painter)
{
    //data for drawing histogram
    float maxValue = log2(getHighestValue());
    int tmpMax = getHighestValue();
    if(this->maxYValueCheckBox->checkState() == Qt::Unchecked)
    {
        maxValue = log2(this->maxYVal);
    }
    float barWidth = (this->paintArea->width()/ (this->histoData.length()*1.0f));

    //drawing histogram
    for (int i = 0; i < this->histoData.length(); i++)
    {
        float logarithm = log2(this->histoData.at(i));
        if(histoData.at(i) == 0)
        {
            logarithm = 0;
        }
        int scaled = (logarithm * (150/maxValue)); //scale values to 150
        int rectX = (barWidth*i)+14;
        int rectY = this->paintArea->height()-scaled;
        int rectHeight = this->paintArea->height()-rectY;
        painter->fillRect(rectX, rectY,barWidth, rectHeight-1, Qt::red);

        // save highest y- coordinate for labeling later

        if((this->histoData.at(i) == tmpMax) && (this->maxYValueCheckBox->checkState() == Qt::Checked))
        {
            this->yCoordMaxLabelLog = rectY;
        }else if(this->maxYValueCheckBox->checkState() == Qt::Unchecked)
        {
            this->yCoordMaxLabelLog = 0;
        }
    }
}

void HistogramWidget::drawAxes(QPainter *painter)
{
    int origX = this->x();
    int rightLimit = this->paintArea->width();
    int bottomLimit = this->paintArea->height();

    //y-Axis
    painter->drawLine(origX+13,bottomLimit-1,origX+13,0);

    //x-Axis
    painter->drawLine(origX+13,bottomLimit-1, rightLimit+13, bottomLimit-1);


}

void HistogramWidget::drawAxesLabel(QPainter *painter)
{
    //set font size for axis labeling
    QFont font;
    font.setPixelSize(8);
    painter->setFont(font);

    //data for drawing
    int maxVal = this->getHighestValue();
    int origX = this->x();
    int rightLimit = this->paintArea->width();
    int bottomLimit = this->paintArea->height();
    int lowerBound = this->histogram->getLowerBound();
    int upperBound = this->histogram->getUpperBound();


    //labeling for normal histogram and automatic highest y value
    if((this->logCheckBox->checkState() == Qt::Unchecked) && (this->maxYValueCheckBox->checkState() == Qt::Checked))
    {
        //y -Axis labeling
        int middleYCoordinate = this->yCoordMaxLabel+5 + (bottomLimit-yCoordMaxLabel)/2;
        painter->drawText(origX, this->yCoordMaxLabel+5, QString::number(maxVal)); //draw maxValue
        if(this->yCoordMaxLabel <= middleYCoordinate-8)// draw middle value only when it does not overlap
        {
            painter->drawText(origX, middleYCoordinate, QString::number(maxVal/2));
        }
        if(middleYCoordinate <= bottomLimit-8) // draw min value only when it does not overlap
        {
            painter->drawText(origX, bottomLimit, "0");
        }
        //x-axis labeling
        painter->drawText(origX+10, bottomLimit+8, QString::number(lowerBound)); //draw left value
        painter->drawText(rightLimit/2-5, bottomLimit+8, QString::number((float)upperBound/2.0)); //draw middle value
        painter->drawText(rightLimit, bottomLimit+8, QString::number(upperBound)); //draw right value
    }

    //labeling for log view histogram and automatic highest y value
    if((this->logCheckBox->checkState() == Qt::Checked) && (this->maxYValueCheckBox->checkState() == Qt::Checked))
    {
        //y -Axis labeling
        int middleYCoordinate = this->yCoordMaxLabelLog+5 + (bottomLimit-this->yCoordMaxLabelLog)/2;
        painter->drawText(origX, this->yCoordMaxLabelLog+5, QString::number(maxVal)); //draw maxValue
        if(this->yCoordMaxLabelLog <= middleYCoordinate-8)// draw middle value only when it does not overlap
        {
            int middleVal = exp2((log2(maxVal)/2));
            painter->drawText(origX, middleYCoordinate, QString::number(middleVal));
        }
        if(middleYCoordinate <= bottomLimit-8)// draw min value only when it does not overlap
        {
            painter->drawText(origX, bottomLimit, "0");
        }

        //x-axis labeling
        painter->drawText(origX+10, bottomLimit+8, QString::number(lowerBound)); //draw left value
        painter->drawText(rightLimit/2-5, bottomLimit+8, QString::number((float)upperBound/2.0)); //draw middle value
        painter->drawText(rightLimit, bottomLimit+8, QString::number(upperBound)); //draw right value
    }

    //labeling for normal histogram and manual highest y value
    if((this->logCheckBox->checkState() == Qt::Unchecked) && (this->maxYValueCheckBox->checkState() == Qt::Unchecked))
    {
        //y -Axis labeling
        int middleYCoordinate = this->yCoordMaxLabel+5 + (bottomLimit-yCoordMaxLabel)/2;
        painter->drawText(origX, this->yCoordMaxLabel+5, QString::number(this->maxYVal)); //draw maxValue
        if(this->yCoordMaxLabel <= middleYCoordinate-8)// draw middle value only when it does not overlap
        {
            painter->drawText(origX, middleYCoordinate, QString::number(this->maxYVal/2));
        }
        if(middleYCoordinate <= bottomLimit-8) // draw min value only when it does not overlap
        {
            painter->drawText(origX, bottomLimit, "0");
        }
        //x-axis labeling
        painter->drawText(origX+10, bottomLimit+8, QString::number(lowerBound)); //draw left value
        painter->drawText(rightLimit/2-5, bottomLimit+8, QString::number((float)upperBound/2.0)); //draw middle value
        painter->drawText(rightLimit, bottomLimit+8, QString::number(upperBound)); //draw right value
    }

    //labeling for log view histogram and manual highest y value
    if((this->logCheckBox->checkState() == Qt::Checked) && (this->maxYValueCheckBox->checkState() == Qt::Unchecked))
    {
        //y -Axis labeling
        int middleYCoordinate = this->yCoordMaxLabelLog+5 + (bottomLimit-this->yCoordMaxLabelLog)/2;
        painter->drawText(origX, this->yCoordMaxLabelLog+5, QString::number(this->maxYVal)); //draw maxValue
        if(this->yCoordMaxLabelLog <= middleYCoordinate-8)// draw middle value only when it does not overlap
        {
            int middleVal = exp2((log2(this->maxYVal)/2));
            painter->drawText(origX, middleYCoordinate, QString::number(middleVal));
        }
        if(middleYCoordinate <= bottomLimit-8)// draw min value only when it does not overlap
        {
            painter->drawText(origX, bottomLimit, "0");
        }

        //x-axis labeling
        painter->drawText(origX+10, bottomLimit+8, QString::number(lowerBound)); //draw left value
        painter->drawText(rightLimit/2-5, bottomLimit+8, QString::number((float)upperBound/2.0)); //draw middle value
        painter->drawText(rightLimit, bottomLimit+8, QString::number(upperBound)); //draw right value
    }


}

int HistogramWidget::getHighestValue()
{
    int maxValue = 1;
    for (int i = 0; i < this->histoData.length(); i++)
    {
        if(maxValue < this->histoData.at(i))
        {
            maxValue = this->histoData.at(i);
        }
    }
    return maxValue;
}



void HistogramWidget::calculateHistogram(FloatImage image)
{

    HistogramWorker* worker = new HistogramWorker(image, histogram);
    connect(worker, &HistogramWorker::histogramCalculated, this, &HistogramWidget::histogramCalculated);
    worker->setAutoDelete(true);
    QThreadPool::globalInstance()->start(worker);
}

void HistogramWidget::setHistogramData(QVector<int> histoData)
{
    this->histoData = histoData;
}

void HistogramWidget::openSettingsDialog()
{
    if(this->dialog == nullptr) //if not yet created, then do it
    {
        this->dialog = new QDialog(this);
        QVBoxLayout *topLayout = new QVBoxLayout(this);

        QHBoxLayout *layout1 = new QHBoxLayout(this);
        layout1->addWidget(new QLabel("Lower bound"));
        this->spinLowerBound = new QDoubleSpinBox();
        this->spinLowerBound->setDecimals(3);
        this->spinLowerBound->setMinimum(std::numeric_limits<int>::lowest());
        this->spinLowerBound->setMaximum(std::numeric_limits<int>::max());
        this->spinLowerBound->setSingleStep(1);
        this->spinLowerBound->setValue(this->histogram->getLowerBound());
        layout1->addWidget(this->spinLowerBound);
        topLayout->addLayout(layout1);


        QHBoxLayout *layout2 = new QHBoxLayout(this);
        layout2->addWidget(new QLabel("Upper bound"));
        this->spinUpperBound = new QDoubleSpinBox();
        this->spinUpperBound->setDecimals(3);
        this->spinUpperBound->setMinimum(std::numeric_limits<int>::lowest());
        this->spinUpperBound->setMaximum(std::numeric_limits<int>::max());
        this->spinUpperBound->setSingleStep(1);
        this->spinUpperBound->setValue(this->histogram->getUpperBound());
        layout2->addWidget(this->spinUpperBound);
        topLayout->addLayout(layout2);

        QHBoxLayout *layout3 = new QHBoxLayout(this);
        //checkBox is created in constructor
        this->spinMaxYValue = new QSpinBox();
        this->spinMaxYValue->setMinimum(1);
        this->spinMaxYValue->setMaximum(std::numeric_limits<int>::max());
        this->spinMaxYValue->setSingleStep(1);
        this->spinMaxYValue->setValue(1);

        layout3->addWidget(new QLabel("Maximum value"));
        layout3->addWidget(this->spinMaxYValue);
        layout3->addWidget(this->maxYValueCheckBox);

        topLayout->addLayout(layout3);


        QPushButton *okButton = new QPushButton("Ok");
        connect(okButton, &QPushButton::clicked, this, &HistogramWidget::updateSettings);
        topLayout->addWidget(okButton);

        this->dialog->setLayout(topLayout);
    }else{
        // dialog still created, so update dilaog options
        this->spinLowerBound->setValue(this->histogram->getLowerBound());
        this->spinUpperBound->setValue(this->histogram->getUpperBound());
        this->spinMaxYValue->setValue(this->maxYVal);

    }
    dialog->exec();
}

void HistogramWidget::updateSettings()
{
    this->histogram->setLowerBound(this->spinLowerBound->value());
    this->histogram->setUpperBound(this->spinUpperBound->value());
    this->maxYVal = this->spinMaxYValue->value();

    dialog->accept();
    emit this->histogramSettingsChanged();
}


void HistogramWidget::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);
    this->histogram->setResolution(this->paintArea->width());
    emit this->histogramSettingsChanged();
}

void HistogramWidget::histogramCalculated(QVector<int> histoData)
{
    this->setHistogramData(histoData);
    this->update();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
