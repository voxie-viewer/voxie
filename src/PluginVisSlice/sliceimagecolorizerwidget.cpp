#include "sliceimagecolorizerwidget.hpp"

#include <PluginVisSlice/colorizerworker.hpp>
#include <PluginVisSlice/makehandbutton.hpp>
#include <PluginVisSlice/slicevisualizer.hpp>

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QThreadPool>

#include <QtGui/QDoubleValidator>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>

using namespace voxie::data;

class NumberValidator : public QValidator
{
public:
    NumberValidator(QObject * parent = 0) : QValidator(parent) {}
    QValidator::State validate(QString &s, int &i) const
    {
		Q_UNUSED(i);
        if (s.isEmpty() || s == "-" || s.toLower() == "n" || s.toLower() == "na") {
            return QValidator::Intermediate;
        }
        if(s.toLower() == "nan")
        {
            return QValidator::Acceptable;
        }
        bool success = false;
        s.toDouble(&success);
        return success ? QValidator::Acceptable : QValidator::Invalid;
    }
};

SliceImageColorizerWidget::SliceImageColorizerWidget(QWidget *parent) :
	QWidget(parent),
    colorizerInstance(this),
	requestColorizer(false)
{
    colorizerInstance.putMapping(0,qRgba(0,0,0,255));
    colorizerInstance.putMapping(1,qRgba(255,255,255,255));
    QString name = "Colorizer";
    this->setWindowTitle(name);

	this->layout = new QVBoxLayout(this);

	this->topRow = new QHBoxLayout();

	this->currentValue = 0.0f;
	this->currentColor = QColor(255,0,255);

	this->colorWidget = new MakeHandButton(this);
	this->colorWidget->setToolTip("Choose Color");
	this->colorWidget->setFixedSize(40, 25);
	this->colorWidget->setStyleSheet("QWidget { background-color: "+this->currentColor.name()+" } QPushButton { border: none }");

	this->currentValueInput = new QLineEdit(this);
    this->currentValueInput->setText(QString::number(this->currentValue));
    this->currentValueInput->setValidator(new NumberValidator(this->currentValueInput));//new MyValidator(0.0, 1.0, 3, currentValueInput));
	this->currentValueInput->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

	this->colorPicker = new QColorDialog();
    this->colorPicker->setOption(QColorDialog::ShowAlphaChannel);

	this->addGradientButton = new MakeHandButton(this);
	this->addGradientButton->setToolTip("Add Mapping");
	this->addGradientButton->setIcon(QIcon(":/icons/plus-small.png"));

	this->line = new QFrame(this);
	this->line->setObjectName(QString::fromUtf8("line"));
	//this->line->setGeometry(QRect(320, 150, 118, 3));
	this->line->setFrameShape(QFrame::HLine);
	this->line->setFrameShadow(QFrame::Sunken);


	this->topRow->addWidget(this->colorWidget);
	this->topRow->addWidget(this->currentValueInput);
	this->topRow->addWidget(this->addGradientButton);

	this->layout->addLayout(this->topRow);
	this->layout->addWidget(this->line);

	connect(this->currentValueInput, &QLineEdit::editingFinished, [=]() {
        float value;
        if(this->currentValueInput->text().toLower() == "nan") {
            value = NAN;
        } else {
            value = this->currentValueInput->text().toFloat();
        }
        this->currentValue = value;
	});
	//connect(this->currentValueInput, &QLineEdit::returnPressed, this->addGradientButton, &QPushButton::click); // called before editingFinished, thanks QT

	connect(this->colorWidget, &QPushButton::clicked, [=]() -> void
	{
        this->colorPicker->exec();
        this->currentColor = this->colorPicker->selectedColor();
        this->colorWidget->setStyleSheet("QWidget {background: "+this->currentColor.name()+"} QPushButton {border: none}");
	});

	connect(this->addGradientButton, &QPushButton::clicked, [=]() -> void
	{
        addMapping(this->currentValue, this->currentColor.rgba());
	});

    //this->box = new Q;
    //this->box->setStyleSheet("QListWidget::item:selected { background: palette(Base) }");
    //box->setStyleSheet("QTreeView:selected {background-color: }");
    //layout->addWidget(box);
    updateButtons();
}

SliceImageColorizerWidget::~SliceImageColorizerWidget()
{
	//delete filter;
    delete colorPicker;
}

void SliceImageColorizerWidget::doColorizer(voxie::data::FloatImage image)
{
	if(!requestColorizer) {
		//qDebug() << "Rendering slice now";
		ColorizerWorker* worker = new ColorizerWorker(image, &colorizerInstance);
		connect(worker, &ColorizerWorker::imageColorized, this, &SliceImageColorizerWidget::imageColorized);
        //connect(worker, &ColorizerWorker::imageColorized, worker, &ColorizerWorker::deleteLater);
        worker->setAutoDelete(true);
		connect(worker, &ColorizerWorker::imageColorized, [=]() -> void {
			if(requestColorizer) {
                //qDebug() << "Rendering slice finished but rendering was requested, rendering again";
				requestColorizer = false;
				this->doColorizer(image);
			}
		});
		QThreadPool::globalInstance()->start(worker);
	} else {
        //qDebug() << "Rendering slice running but rendering requested";
		requestColorizer = true;
	}
}
void SliceImageColorizerWidget::updateButtons() {
	//qDebug() << "DISABLED";
	this->setUpdatesEnabled(false);
	this->hide();
    emit deleteOldButtons();
    for (QPair<float, QRgb> mapping : colorizerInstance.getMappings()) {
        float in = mapping.first;
        QRgb out = mapping.second;
        QColor color(out);
        QHBoxLayout* row = new QHBoxLayout;

        MakeHandButton* removeMappingButton = new MakeHandButton();
		removeMappingButton->setToolTip("Remove Mapping");
		removeMappingButton->setIcon(QIcon(":/icons/cross-small.png"));

        QLineEdit* inValue = new QLineEdit;

        inValue->setValidator(new NumberValidator(inValue));//MyValidator(0.0, 1.0, 3, inValue));
        inValue->setText( QString::number(in));
		inValue->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

        connect(inValue, &QLineEdit::editingFinished, [=]() {
			if(this->updatesEnabled()) {
                float value;
                if(inValue->text().toLower() == "nan") {
                     value = colorizerInstance.getNanColor();
                } else {
                     value = inValue->text().toFloat();
                }
                updateMapping(in,value, out);
			}
        });
        connect(removeMappingButton, &QPushButton::clicked, [=]() {
            removeMapping(in);
        });

		MakeHandButton* colorWidgetRow = new MakeHandButton();
		colorWidgetRow->setToolTip("Choose Color");
		colorWidgetRow->setFixedSize(40, 25);
		colorWidgetRow->setStyleSheet("QWidget {background: "+color.name()+"} QPushButton {border: none}");

		connect(colorWidgetRow, &QPushButton::clicked, [=]() -> void
		{
            if(this->updatesEnabled()) {
                this->colorPicker->setCurrentColor(QColor(qRed(out), qGreen(out), qBlue(out), qAlpha(out)));
                if(this->colorPicker->exec() == QDialog::Accepted)
                    updateMapping(in, in, this->colorPicker->selectedColor().rgba());
            }
		});

		row->addWidget(colorWidgetRow);
		row->addWidget(inValue);
		row->addWidget(removeMappingButton);
        layout->addLayout(row);

        connect(this, &SliceImageColorizerWidget::deleteOldButtons, row, &QHBoxLayout::deleteLater);
        connect(this, &SliceImageColorizerWidget::deleteOldButtons, inValue, &QLineEdit::hide);
        connect(this, &SliceImageColorizerWidget::deleteOldButtons, inValue, &QLineEdit::deleteLater);
        connect(this, &SliceImageColorizerWidget::deleteOldButtons, removeMappingButton, &MakeHandButton::hide);
        connect(this, &SliceImageColorizerWidget::deleteOldButtons, removeMappingButton, &MakeHandButton::deleteLater);
		connect(this, &SliceImageColorizerWidget::deleteOldButtons, colorWidgetRow, &MakeHandButton::hide);
        connect(this, &SliceImageColorizerWidget::deleteOldButtons, colorWidgetRow, &MakeHandButton::deleteLater);
    }
	this->setVisible(true);
	this->setUpdatesEnabled(true);
	//qDebug() << "ENABLED";
}

void SliceImageColorizerWidget::updateMapping(float old, float in, QRgb out) {
    //qDebug() << "updateMapping";
    colorizerInstance.removeMapping(old);
    colorizerInstance.putMapping(in, out);
    emit gradientChanged(colorizerInstance.getColorMap());
    updateButtons();
}

void SliceImageColorizerWidget::removeMapping(float in)
{
    //qDebug() << "removeMapping";
    colorizerInstance.removeMapping(in);
    emit gradientChanged(colorizerInstance.getColorMap());
    updateButtons();
}

void SliceImageColorizerWidget::addMapping(float in, QRgb out)
{
    //qDebug() << "addMapping";
    if(isnan(in)){
        colorizerInstance.setNanColor(out);
    } else {
        colorizerInstance.putMapping(in, out);
    }
    emit gradientChanged(colorizerInstance.getColorMap());
    updateButtons();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
