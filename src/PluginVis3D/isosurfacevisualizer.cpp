#include "isosurfacevisualizer.hpp"

#include <QtOpenGL/QGLFormat>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::data;
using namespace voxie::visualization;

IsosurfaceVisualizer::IsosurfaceVisualizer(DataSet *data, QWidget *parent) :
	VolumeDataVisualizer(parent),
	voxelData_(data)
{
	this->setObjectName(data->objectName() + "_isosurface");
	this->setWindowTitle(data->objectName() + " - Isosurface");
	this->setMinimumSize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    {
        this->view = new IsosurfaceView(data);
        this->view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(this->view);
    }
    this->setLayout(layout);


    {
        QWidget *sectionControl = new QWidget();
        sectionControl->setWindowTitle("3D Settings");
        {
            QFormLayout *controlLayout = new QFormLayout();
            {
                QPushButton *refreshButton;


                auto range = this->dataSet()->filteredData()->getMinMaxValue();
                float position = range.first + 0.5f * (range.second - range.first);

                this->view->threshold = position;

                controlLayout->addRow("Threshold", (this->thresholdEdit = new QLineEdit(QString::number(position))));
                connect(this->thresholdEdit, &QLineEdit::textChanged, this, &IsosurfaceVisualizer::refreshSlider);

                controlLayout->addRow("", (this->thresholdSlider = new QSlider(Qt::Horizontal)));
                this->thresholdSlider->setMinimum(0);
                this->thresholdSlider->setMaximum(1000);
                this->thresholdSlider->setValue(500);
                connect(this->thresholdSlider, &QSlider::valueChanged, this, &IsosurfaceVisualizer::refreshTextEdit);

                controlLayout->addRow("Method", (this->methodBox = new QComboBox()));
                this->methodBox->addItem("Cuberille");
                this->methodBox->addItem("Marching Cubes");

                controlLayout->addRow("Invert", (this->invertedCheck = new QCheckBox()));
                this->invertedCheck->setChecked(this->view->inverted);

                controlLayout->addRow("", (refreshButton = new QPushButton("Refresh")));
                connect(refreshButton, &QPushButton::clicked, this, &IsosurfaceVisualizer::refresh3D);
            }
            sectionControl->setLayout(controlLayout);
        }
        this->dynamicSections().append(sectionControl);
    }
}

void IsosurfaceVisualizer::refreshTextEdit(int position)
{
    auto range = this->dataSet()->filteredData()->getMinMaxValue();

    float value = range.first + 0.001f * position * (range.second - range.first);
    this->thresholdEdit->setText(QString::number(value));
}

void IsosurfaceVisualizer::refreshSlider(const QString &text)
{
    Q_UNUSED(text);
    bool ok;
    auto range = this->dataSet()->filteredData()->getMinMaxValue();
    float threshold = this->thresholdEdit->text().toFloat(&ok);
    if(ok)
    {
        float location = (threshold - range.first) / (range.second - range.first);
        if(location < 0) location = 0;
        if(location > 1) location = 1;
        this->thresholdSlider->setValue(1000 * location);
    }
}

void IsosurfaceVisualizer::refresh3D()
{
    bool ok;
    auto range = this->dataSet()->filteredData()->getMinMaxValue();
    float threshold = this->thresholdEdit->text().toFloat(&ok);
    if(ok)
    {
        this->view->threshold = threshold;

        float location = (threshold - range.first) / (range.second - range.first);
        if(location < 0) location = 0;
        if(location > 1) location = 1;

        this->thresholdSlider->setValue(1000 * location);
    }
    this->view->inverted = this->invertedCheck->isChecked();
    this->view->useMarchingCubes = (this->methodBox->currentIndex() == 1);
    this->view->regenerate();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
