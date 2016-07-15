#include "xrayvisualizer.hpp"

#include <Voxie/spnav/spacenavvisualizer.hpp>

#include <Voxie/opencl/clinstance.hpp>
#include <Voxie/opencl/clutil.hpp>
#include <Voxie/scripting/scriptingexception.hpp>

#include <PluginVis3D/xraymetavisualizer.hpp>

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

using namespace voxie::data;
using namespace voxie::opencl;

XRayVisualizer::XRayVisualizer(DataSet *dataSet, QWidget *parent) : VolumeDataVisualizer (parent), dataSet_ (dataSet) {
    this->setDisplayName(dataSet->displayName() + " - XRay");

    view = new XRayView(this);

    connect(this->dataSet_, &QObject::destroyed, this, [this]() -> void {
        this->dataSet_ = nullptr;
    });

    this->dynamicSections().append(view->sidePanel);

    auto sn = new voxie::spnav::SpaceNavVisualizer(this);
    view->view3d->registerSpaceNavVisualizer(sn);
}

voxie::plugin::MetaVisualizer* XRayVisualizer::type() const {
    return XRayMetaVisualizer::instance();
}

XRayView::XRayView(XRayVisualizer* visualizer) :
    visualizer(visualizer),
    //view3d(new voxie::visualization::View3D(this, this->dataSet()->diagonalSize()))
    view3d(new voxie::visualization::View3D(this, true, 2))
{
    if (!CLInstance::getDefaultInstance()->isValid())
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.NoOpenCLSupport", "OpenCL support not available but needed for XRay visualizer");
    if (this->dataSet()->filteredData()->getCLImage()() == nullptr)
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.NoOpenCLImage", "Upload of data to GPU failed");
    // TODO: handle case when a filter gets added later and the filtered dataset
    // cannot be uploaded to the GPU?

    this->setWindowTitle(dataSet()->displayName() + " - XRay");
    this->kernel = CLInstance::getDefaultInstance()->getKernel("voxie3d::x-ray-3d", "render");

    sidePanel = new QWidget();
    {
        sidePanel->setWindowTitle("X-Ray Settings");
        QFormLayout *layout = new QFormLayout();
        {
            QHBoxLayout *qualities = new QHBoxLayout();
            {
                qualities->addWidget(this->radioQ0 = new QRadioButton("32"));
                qualities->addWidget(this->radioQ1 = new QRadioButton("64"));
                qualities->addWidget(this->radioQ2 = new QRadioButton("128"));
                qualities->addWidget(this->radioQ3 = new QRadioButton("256"));
                this->radioQ1->setChecked(true);

                connect(this->radioQ0, &QRadioButton::toggled, this, &XRayView::updateButton);
                connect(this->radioQ1, &QRadioButton::toggled, this, &XRayView::updateButton);
                connect(this->radioQ2, &QRadioButton::toggled, this, &XRayView::updateButton);
                connect(this->radioQ3, &QRadioButton::toggled, this, &XRayView::updateButton);
            }
            layout->addRow(new QLabel("Render Quality"), qualities);

            layout->addRow("Minimum", this->minSlider = new QSlider());
            layout->addRow("Maximum", this->maxSlider = new QSlider());
            layout->addRow("Result Scale", this->scaleSlider = new QSlider());

            this->minSlider->setMinimum(0);
            this->minSlider->setMaximum(1000);
            this->minSlider->setValue(0);
            this->minSlider->setOrientation(Qt::Horizontal);
            connect(this->minSlider, &QSlider::valueChanged, this, &XRayView::updateSlider);

            this->maxSlider->setMinimum(0);
            this->maxSlider->setMaximum(1000);
            this->maxSlider->setValue(1000);
            this->maxSlider->setOrientation(Qt::Horizontal);
            connect(this->maxSlider, &QSlider::valueChanged, this, &XRayView::updateSlider);

            this->scaleSlider->setMinimum(0);
            this->scaleSlider->setMaximum(3000);
            this->scaleSlider->setValue(1000);
            this->scaleSlider->setOrientation(Qt::Horizontal);
            connect(this->scaleSlider, &QSlider::valueChanged, this, &XRayView::updateSlider);
        }
        sidePanel->setLayout(layout);
    }

    this->setMinimumSize(300,200);

    connect(view3d, &voxie::visualization::View3D::changed, this, [this] { this->update(); });
}

void XRayView::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter painter(this);
    QString message = "Success";
    //--------------------------------------------------------------------------------//
    if(this->dataSet() != nullptr)
    {
        try{
        cl_int eastwood;
        cl::Device device = CLInstance::getDefaultInstance()->getDevice(CL_DEVICE_TYPE_GPU);
        cl::CommandQueue queue = CLInstance::getDefaultInstance()->getCommandQueue(device);

        QVector3D spacing = this->dataSet()->filteredData()->getSpacing().normalized();
        cl_float3 transform =
        {{
            spacing.x(),
            spacing.y(),
            spacing.z()
        }};
        transform =
        {{
                1,1,1
        }};

        QMatrix4x4 invViewProjectionMatrix =  (view3d->projectionMatrix(this->image.width(), this->image.height()) * view3d->viewMatrix()).inverted();
        cl_float16 invViewProjection = {{
                invViewProjectionMatrix(0, 0),
                invViewProjectionMatrix(0, 1),
                invViewProjectionMatrix(0, 2),
                invViewProjectionMatrix(0, 3),
                invViewProjectionMatrix(1, 0),
                invViewProjectionMatrix(1, 1),
                invViewProjectionMatrix(1, 2),
                invViewProjectionMatrix(1, 3),
                invViewProjectionMatrix(2, 0),
                invViewProjectionMatrix(2, 1),
                invViewProjectionMatrix(2, 2),
                invViewProjectionMatrix(2, 3),
                invViewProjectionMatrix(3, 0),
                invViewProjectionMatrix(3, 1),
                invViewProjectionMatrix(3, 2),
                invViewProjectionMatrix(3, 3),
            }};

        cl_int quality = 32;
        if(this->radioQ1->isChecked()) quality = 64;
        if(this->radioQ2->isChecked()) quality = 128;
        if(this->radioQ3->isChecked()) quality = 256;

        QPair<Voxel, Voxel> minMax = this->dataSet()->filteredData()->getMinMaxValue();
        cl_float2 voxelRange =
        {{
             minMax.first + (minMax.second - minMax.first) * 0.001f * this->minSlider->value(),
             minMax.first + (minMax.second - minMax.first) * 0.001f * this->maxSlider->value()
        }};

        float scale = 0.001f * this->scaleSlider->value();
        scale *= scale;

        this->kernel.setArg(0, this->clImage);
        this->kernel.setArg(1, this->dataSet()->filteredData()->getCLImage());
        this->kernel.setArg(2, transform);
        //this->kernel.setArg(3, invViewProjection);
        this->kernel.setArg(3, sizeof (cl_float16), &invViewProjection);
        this->kernel.setArg(4, voxelRange);
        this->kernel.setArg(5, quality);
        this->kernel.setArg(6, scale);


        eastwood = queue.enqueueNDRangeKernel(
                    this->kernel,
                    cl::NullRange,
                    cl::NDRange(this->image.width(), this->image.height(), 1),
                    cl::NullRange);
        if(eastwood != CL_SUCCESS)
        {
            message = clErrorToString(eastwood);
        }

        cl::size_t<3> origin, area;
        area[0] = this->image.width();
        area[1] = this->image.height();
        area[2] = 1;
        queue.enqueueReadImage(
                    this->clImage,
                    CL_TRUE,
                    origin,
                    area,
                    this->image.bytesPerLine(), 0,
                    this->image.scanLine(0));
        } catch(CLException &ex){
            qWarning() << ex;
            message = clErrorToString(ex.getErrorCode());
        }
    }
    //--------------------------------------------------------------------------------//
    painter.drawImage(0, 0, this->image);
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(16, 16, message);
}


void XRayView::resizeEvent(QResizeEvent *event)
{
    if(event->size() == event->oldSize())
        return; // If this happens, something weird has happend

    if(event->size().isEmpty())
        return;

    this->image = QImage(this->width(), this->height(), QImage::Format_RGBA8888);
    this->image.fill(QColor(0, 0, 0, 0));
    try {
    this->clImage = CLInstance::getDefaultInstance()->createImage2D(
                cl::ImageFormat(CL_RGBA, CL_UNORM_INT8),
                this->image.width(),
                this->image.height(),
                nullptr);
    } catch(CLException &ex){
        qWarning() << ex;
    }

    this->update();
}
void XRayView::mousePressEvent(QMouseEvent *event)
{
    view3d->mousePressEvent(mouseLast, event, size());
    this->mouseLast = event->pos();
}

void XRayView::mouseMoveEvent(QMouseEvent *event)
{
    view3d->mouseMoveEvent(mouseLast, event, size());
    this->mouseLast = event->pos();
}

void XRayView::wheelEvent(QWheelEvent *event)
{
    view3d->wheelEvent(event, size());
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
