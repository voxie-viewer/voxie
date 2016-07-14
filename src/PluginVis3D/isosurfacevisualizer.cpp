#include "isosurfacevisualizer.hpp"

#include <Voxie/spnav/spacenavvisualizer.hpp>

#include <Voxie/data/surface.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <Voxie/io/savefiledialog.hpp>

#include <QtOpenGL/QGLFormat>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#ifdef ENABLE_OSVR
#include <PluginVis3D/osvr/osvrdisplay.hpp>
#endif

using namespace voxie::data;
using namespace voxie::visualization;

IsosurfaceVisualizer::IsosurfaceVisualizer(DataSet *data, QWidget *parent) :
	VolumeDataVisualizer(parent),
	voxelData_(data)
{
    bool autoRegenerate = false;

    this->setDisplayName(data->displayName() + " - Isosurface");

    {
        this->view = new IsosurfaceView(data);
        this->view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        this->view->setWindowTitle(data->displayName() + " - Isosurface");
        this->view->setMinimumSize(400, 300);
    }


    QFormLayout *controlLayout;
    {
        QWidget *sectionControl = new QWidget();
        sectionControl->setWindowTitle("3D Settings");
        {
            controlLayout = new QFormLayout();
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
                this->methodBox->setCurrentIndex(1);

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
                    controlLayout->addRow("Save", (saveButton = new QPushButton("Save current surface...")));
                    connect(saveButton, &QPushButton::clicked, this, &IsosurfaceVisualizer::saveAs);
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

#ifdef ENABLE_OSVR
    auto osvrEnabled = new QCheckBox();
    controlLayout->addRow("OSVR", osvrEnabled);
    connect(osvrEnabled, &QCheckBox::stateChanged, this, [this, osvrEnabled] (int state) {
            if (!state) {
                if (!this->osvrDisplay)
                    return;
                osvrDisplay->deleteLater();
            } else {
                if (this->osvrDisplay)
                    return;
                try {
                    auto osvr = new OsvrDisplay(view->context()->contextHandle(), view, this);
                    this->osvrDisplay = osvr;
                    connect(osvr, &QObject::destroyed, osvrEnabled, [osvrEnabled] {
                            osvrEnabled->setChecked(false);
                        });
                    connect(osvr, &OsvrDisplay::render, this, [this] (const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix) {
                            QMatrix4x4 matViewProj = projectionMatrix * viewMatrix * view->getView3D()->viewMatrix();
                            view->paint(matViewProj);
                        });
                } catch (voxie::scripting::ScriptingException& e) {
                    qCritical() << "Failed to create OsvrDisplay:" << e.message();
                }
            }
        });

#if !defined(Q_OS_WIN)
    QPushButton* toggleSideBySide;
    controlLayout->addRow("", (toggleSideBySide = new QPushButton("Toggle side-by-side")));
    connect(toggleSideBySide, &QPushButton::clicked, this, [this] {
            QFile file("/dev/ttyUSB.OSVRHDK");
            if (file.open(QIODevice::ReadWrite)) {
                const char* str = "\n#f1s\n";
                qint64 len = strlen(str);
                if (file.write(str, len) != len) {
                    QMessageBox(QMessageBox::Critical, "OSVR", QString("Error while writing to %1").arg(file.fileName()), QMessageBox::Ok, view).exec();
                }
            } else {
                QMessageBox(QMessageBox::Critical, "OSVR", QString("Failed to open %1").arg(file.fileName()), QMessageBox::Ok, view).exec();
            }
        });
#endif

#endif

    refresh3D();

    auto sn = new voxie::spnav::SpaceNavVisualizer(this);
    view->getView3D()->registerSpaceNavVisualizer(sn);
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

static inline void write8(QFile& file, quint8 value) {
    write(file, &value, 1);
}
static inline void write16(QFile& file, quint16 value) {
    write(file, &value, 2);
}
static inline void write32(QFile& file, quint32 value) {
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

void IsosurfaceVisualizer::saveToSTL(const Surface* surface, const QString& filename) {
    QFile stlFile(filename);
    stlFile.open(QIODevice::WriteOnly);

    char header[80];
    memset (header, 0, 80);
    write(stlFile, header, 80);
    if (surface->triangles().size() > std::numeric_limits<quint32>::max())
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Cannot write data to STL file: More than 2^32-1 triangles");
    quint32 count = surface->triangles().size();
    write32(stlFile, count);

    for (size_t i = 0; i < surface->triangles().size(); i++) {
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
        write16(stlFile, attrcount);
    }

    if (!stlFile.flush())
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IO.WriteError", "Error while flushing STL file: " + stlFile.errorString());

    stlFile.close();
}

// https://en.wikipedia.org/w/index.php?title=PLY_(file_format)&oldid=717727643
void IsosurfaceVisualizer::saveToPLY(const Surface* surface, const QString& filename) {
    QFile plyFile(filename);
    plyFile.open(QIODevice::WriteOnly);

    {
        QTextStream out(&plyFile);
        out.setRealNumberPrecision(9);

        out << "ply\n";
        if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
            out << "format binary_big_endian 1.0\n";
        } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
            out << "format binary_little_endian 1.0\n";
        } else {
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IO.WriteError", "Unknown endianess");
        }

        out << "comment Generated by Voxie\n";

        out << "element vertex " << surface->vertices().size() << "\n";
        out << "property float32 x\n";
        out << "property float32 y\n";
        out << "property float32 z\n";

        out << "element face " << surface->triangles().size() << "\n";
        out << "property list uint8 uint32 vertex_indices\n";

        out << "end_header\n";
    }

    for (const auto& vertex : surface->vertices())
        write(plyFile, vertex);

    for (const auto& triangle : surface->triangles()) {
        write8(plyFile, 3); // Number of vertices
        write32(plyFile, triangle[0]);
        write32(plyFile, triangle[1]);
        write32(plyFile, triangle[2]);
    }

    if (!plyFile.flush())
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IO.WriteError", "Error while flushing PLY file: " + plyFile.errorString());

    plyFile.close();
}

void IsosurfaceVisualizer::saveToFTR(const Surface* surface, const QString& filename) {
    QFile ftrFile(filename);
    ftrFile.open(QIODevice::WriteOnly);

    QTextStream out(&ftrFile);
    out.setRealNumberPrecision(9);

    out << "\n1\n(\n\npatch0\nempty\n)\n\n\n";

    out << surface->vertices().size() << "\n";
    out << "(\n";
    for (const auto& vertex : surface->vertices())
        out << "(" << vertex.x() << " " << vertex.y() << " " << vertex.z() << ")\n";
    out << ")\n\n\n";

    out << surface->triangles().size() << "\n";
    out << "(\n";
    for (const auto& triangle : surface->triangles())
        out << "((" << triangle[0] << " " << triangle[1] << " " << triangle[2] << ") 0)\n";
    out << ")\n\n";

    if (!ftrFile.flush())
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IO.WriteError", "Error while flushing FTR file: " + ftrFile.errorString());

    ftrFile.close();
}

void IsosurfaceVisualizer::saveAs() {
    try {
        QSharedPointer<Surface> surface = view->getSurface();
        if (!surface)
            return;

        typedef std::function<void (const Surface* surface, const QString& filename)> funType;

        voxie::io::SaveFileDialog dialog(view->window(), tr("Save current surface"), QString());

        funType savePly = saveToPLY;
        dialog.addFilter("Stanford PLY", QStringList() << "ply", &savePly);

        funType saveStl = saveToSTL;
        dialog.addFilter("Binary STL", QStringList() << "stl", &saveStl);

        funType saveFtr = saveToFTR;
        dialog.addFilter("Freefoam FTR", QStringList() << "ftr", &saveFtr);

        dialog.setup();
        if (!dialog.exec())
            return;

        void* data = dialog.selectedFilterData();
        if (!data)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "No filter selected");

        QStringList files = dialog.selectedFiles();
        if (files.size() != 1) {
            qWarning() << "QFileDialog returned" << files.size() << "entries";
            return;
        }
        QString filename = files[0];
        //qDebug() << filename;

        (*((funType*) data)) (surface.data(), filename);
    } catch (voxie::scripting::ScriptingException& e) {
        QMessageBox(QMessageBox::Critical, "Error while writing STL file", QString("Error while writing STL file: %1").arg(e.message()), QMessageBox::Ok, view).exec();
        return;
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
