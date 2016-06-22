#include "isosurfacevisualizer.hpp"

#include <Voxie/data/surface.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtOpenGL/QGLFormat>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

using namespace voxie::data;
using namespace voxie::visualization;

IsosurfaceVisualizer::IsosurfaceVisualizer(DataSet *data, QWidget *parent) :
	VolumeDataVisualizer(parent),
	voxelData_(data)
{
    bool autoRegenerate = false;

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
                auto range = this->dataSet()->filteredData()->getMinMaxValue();
                float position = range.first + 0.05f * (range.second - range.first);

                this->view->threshold = position;

                controlLayout->addRow("Threshold", (this->thresholdEdit = new QDoubleSpinBox()));
                this->thresholdEdit->setDecimals(2);
                this->thresholdEdit->setRange(range.first, range.second);
                //this->thresholdEdit->setSuffix(" 1/m");
                this->thresholdEdit->setValue(position);

                controlLayout->addRow("Method", (this->methodBox = new QComboBox()));
                this->methodBox->addItem("Cuberille");
                this->methodBox->addItem("Marching Cubes");

                controlLayout->addRow("Invert", (this->invertedCheck = new QCheckBox()));
                this->invertedCheck->setChecked(this->view->inverted);

                if (!autoRegenerate) {
                    QPushButton *refreshButton;
                    controlLayout->addRow("", (refreshButton = new QPushButton("Refresh")));
                    connect(refreshButton, &QPushButton::clicked, this, &IsosurfaceVisualizer::refresh3D);
                }

                controlLayout->addRow("Culling", (this->culling = new QComboBox()));
                this->culling->addItem("None (show both faces)");
                this->culling->addItem("Show only front faces");
                this->culling->addItem("Show only back faces");
                connect (this->culling, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this->view, [this](int index) {
                        switch (index) {
                        case 0: this->view->culling = IsosurfaceView::NO_CULLING; break;
                        case 1: this->view->culling = IsosurfaceView::SHOW_FRONT; break;
                        case 2: this->view->culling = IsosurfaceView::SHOW_BACK; break;
                        }
                        this->view->update();
                    });

                    QPushButton* saveButton;
                    controlLayout->addRow("Save", (saveButton = new QPushButton("Save current surface as STL...")));
                    connect(saveButton, &QPushButton::clicked, this, &IsosurfaceVisualizer::saveAsSTL);
            }
            sectionControl->setLayout(controlLayout);
        }
        this->dynamicSections().append(sectionControl);
    }

    if (autoRegenerate) {
        connect(this->thresholdEdit, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &IsosurfaceVisualizer::refresh3D);
        connect(this->methodBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &IsosurfaceVisualizer::refresh3D);
        connect(this->invertedCheck, &QCheckBox::stateChanged, this, &IsosurfaceVisualizer::refresh3D);
    }
    refresh3D();
}

void IsosurfaceVisualizer::refresh3D()
{
    this->view->threshold = this->thresholdEdit->value();
    this->view->inverted = this->invertedCheck->isChecked();
    this->view->useMarchingCubes = (this->methodBox->currentIndex() == 1);
    this->view->regenerate();
}

static inline void write(QFile& file, const void* data, int length) {
    int pos = 0;
    while (pos < length) {
        int bytes = file.write(reinterpret_cast<const char*>(data) + pos, length - pos);
        if (bytes <= 0)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IO.WriteError", "Error while writing to STL file: " + file.errorString());
        pos += bytes;
    }
}

static inline void write(QFile& file, quint16 value) {
    write(file, &value, 2);
}
static inline void write(QFile& file, quint32 value) {
    write(file, &value, 4);
}
static inline void write(QFile& file, const QVector3D& vertex) {
    float x = vertex.x();
    float y = vertex.y();
    float z = vertex.z();
    write(file, &x, 4);
    write(file, &y, 4);
    write(file, &z, 4);
}

void IsosurfaceVisualizer::saveAsSTL() {
    try {
        QSharedPointer<Surface> surface = view->getSurface();
        if (!surface)
            return;

        QString filename = QFileDialog::getSaveFileName(nullptr, tr("Save current surface as binary STL"), QDir::currentPath(), "Binary STL file (*.stl)");
        if (filename.isNull())
            return;

        QFile stlFile(filename);
        stlFile.open(QIODevice::WriteOnly);
        char header[80];
        memset (header, 0, 80);
        write(stlFile, header, 80);
        quint32 count = surface->triangles().size();
        write(stlFile, count);
    
        for (int i = 0; i < surface->triangles().size(); i++) {
            const auto& triangle = surface->triangles()[i];
            QVector3D a = surface->vertices()[triangle[0]];
            QVector3D b = surface->vertices()[triangle[1]];
            QVector3D c = surface->vertices()[triangle[2]];

            QVector3D normal = IsosurfaceView::normalized(QVector3D::crossProduct(b - c, c - a));

            write(stlFile, normal);

            write(stlFile, a);
            write(stlFile, b);
            write(stlFile, c);

            quint16 attrcount = 0;
            write(stlFile, attrcount);
        }

        if (!stlFile.flush())
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IO.WriteError", "Error while flushing STL file: " + stlFile.errorString());

        stlFile.close();
    } catch (voxie::scripting::ScriptingException& e) {
        QMessageBox(QMessageBox::Critical, "Error while writing STL file", QString("Error while writing STL file: %1").arg(e.message()), QMessageBox::Ok, this).exec();
        return;
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
