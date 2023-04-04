/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "VolumeRenderingVisualizer.hpp"

#include <Voxie/SpNav/SpaceNavVisualizer.hpp>

#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>
#include <VoxieBackend/OpenCL/CLUtil.hpp>

#include <VoxieClient/Exception.hpp>

#include <PluginVis3D/Prototypes.hpp>
#include <PluginVis3D/VolumeImageRenderer.hpp>

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

#ifdef VOLUME_RENDERING_VISUALIZER_CPP_ADDITIONAL_INCLUDE
#include VOLUME_RENDERING_VISUALIZER_CPP_ADDITIONAL_INCLUDE
#endif

using namespace vx;
using namespace vx::opencl;

// TODO: Add real properties to this visualizer

typedef struct {
  cl_float4 position;
  cl_float4 colorF;
} cl_lightSource;

VolumeRenderingVisualizer::VolumeRenderingVisualizer()
    : VisualizerNode(getPrototypeSingleton()) {
  this->view = nullptr;
  this->mainDataset = nullptr;

  view = new VolumeRenderingView(this);
  this->setAutomaticDisplayName("Volume Visualizer");

  this->objProp = new ObjectProperties(view);
  this->settingUpObjectProperties();

  this->lightSrcProp = new LightSourceProperties(view);
  this->settingUpLightSourceProperties();

  RenderImplementationSelection* renderImpl =
      new RenderImplementationSelection(view);
  this->settingUpRenderImplementationSelection(renderImpl);

  auto sn = new vx::spnav::SpaceNavVisualizer(this);
  view->view3d->registerSpaceNavVisualizer(sn);

  this->dynamicSections().append(objProp);
  this->dynamicSections().append(lightSrcProp);
  this->dynamicSections().append(view->sidePanel);
  this->dynamicSections().append(renderImpl);

  connect(this, &Node::parentChanged, this,
          &VolumeRenderingVisualizer::dataSetChanged);

  connect(view, &VolumeRenderingView::generationDone, view,
          &VolumeRenderingView::reRendering);
}

void VolumeRenderingVisualizer::dataSetChanged(Node* node) {
  if (this->mainDataset != nullptr) {
    this->disconnect(this->mainDataset, 0, this, 0);
  }

  this->setAutomaticDisplayName("Volume Visualizer");

  VolumeNode* dataset = dynamic_cast<VolumeNode*>(node);
  if (!dataset) {  // TODO
    qWarning() << "Not a VolumeNode";
    this->mainDataset = nullptr;
    this->view->update();
    return;
  }

  if (dataset) {
    if (this->parentNodes().size() > 1) {
      this->parentNodes()[0]->removeChildNode(this);
    }

    if (!this->hasParent(dataset)) {
      dataset = nullptr;
      if (!this->parentNodes().isEmpty()) {
        dataset = dynamic_cast<VolumeNode*>(this->parentNodes().at(0));
      }
    }

    if (this->parentNodes().size() > 1) {
      this->parentNodes()[0]->removeChildNode(this);
    }

    if (!this->hasParent(dataset)) {
      dataset = nullptr;
      if (!this->parentNodes().isEmpty()) {
        dataset = dynamic_cast<VolumeNode*>(this->parentNodes().at(0));
      }
    }

    this->mainDataset = dataset;

    if (this->mainDataset) {
      this->setAutomaticDisplayName("Volume Visualizer - " +
                                    this->mainDataset->displayName());
      connect(this->mainDataset, &Node::displayNameChanged, this, [this] {
        this->setAutomaticDisplayName("Volume Visualizer - " +
                                      this->mainDataset->displayName());
      });

      this->settingUpObjectProperties();

      objProp->setPosition(this->mainDataset);
      objProp->setRotation(this->mainDataset);
    }
    view->dataSetChanged();
  } else {
    qWarning() << "Not a VolumeNode";
    this->mainDataset = nullptr;
    this->view->update();
    return;
  }
}

void VolumeRenderingVisualizer::setVolumeMin(int min) { view->setMinimum(min); }

void VolumeRenderingVisualizer::setVolumeMax(int max) { view->setMaximum(max); }

void VolumeRenderingVisualizer::setVolumeScale(int scale) {
  view->setScale(scale);
}

VolumeRenderingView::VolumeRenderingView(VolumeRenderingVisualizer* visualizer)
    : visualizer(visualizer),
      view3d(new vx::visualization::View3D(
          this, vx::visualization::View3DProperty::All)) {
  if (!CLInstance::getDefaultInstance()->isValid()) {
    qWarning()
        << "OpenCL support not available. CPU implementation used instead";
  } else {
    this->kernel = CLInstance::getDefaultInstance()->getKernel(
        "voxie3d::x-ray-3d", "render");
  }

  sidePanel = new QWidget();
  {
    sidePanel->setWindowTitle("Volume Rendering Settings");
    QFormLayout* layout = new QFormLayout();
    {
      QHBoxLayout* qualities = new QHBoxLayout();
      {
        qualities->addWidget(this->radioQ0 = new QRadioButton("32"));
        qualities->addWidget(this->radioQ1 = new QRadioButton("64"));
        qualities->addWidget(this->radioQ2 = new QRadioButton("128"));
        qualities->addWidget(this->radioQ3 = new QRadioButton("256"));
        // qualities
        this->radioQ1->setChecked(true);

        connect(this->radioQ0, &QRadioButton::toggled, this,
                &VolumeRenderingView::updateButton);
        connect(this->radioQ1, &QRadioButton::toggled, this,
                &VolumeRenderingView::updateButton);
        connect(this->radioQ2, &QRadioButton::toggled, this,
                &VolumeRenderingView::updateButton);
        connect(this->radioQ3, &QRadioButton::toggled, this,
                &VolumeRenderingView::updateButton);
      }
      layout->addRow(new QLabel("Render Quality"), qualities);
      layout->addWidget(this->antiAliazingCheckBox =
                            new QCheckBox("Anti-Aliazing"));
      connect(this->antiAliazingCheckBox, &QCheckBox::toggled, this,
              &VolumeRenderingView::setUseAntiAliazing);
      connect(this->antiAliazingCheckBox, &QCheckBox::toggled, this,
              &VolumeRenderingView::updateButton);

      QString qualityTip =
          "Sets the number of rendering samples. Higher values may cause lag.";
      radioQ0->setToolTip(qualityTip);
      radioQ1->setToolTip(qualityTip);
      radioQ2->setToolTip(qualityTip);
      radioQ3->setToolTip(qualityTip);
      antiAliazingCheckBox->setToolTip("Activates/Deactivates Anti-Aliazing");
      antiAliazingCheckBox->setChecked(this->useAntiAliazing);

      QBoxLayout* minLayout = new QHBoxLayout();
      QBoxLayout* maxLayout = new QHBoxLayout();
      QBoxLayout* scaleLayout = new QHBoxLayout();

      layout->addRow("Minimum", minLayout);
      layout->addRow("Maximum", maxLayout);
      layout->addRow("Result Scale", scaleLayout);

      this->minSlider = new QSlider();
      this->minSlider->setMinimum(0);
      this->minSlider->setMaximum(1000);
      this->minSlider->setValue(1);
      this->minSlider->setOrientation(Qt::Horizontal);
      connect(this->minSlider, &QSlider::valueChanged, this,
              &VolumeRenderingView::updateMinSlider);
      minLayout->addWidget(minSlider);
      minLayout->addWidget(
          minField = new QLineEdit(QString::number(minSlider->value())));
      minField->setFixedWidth(40);
      connect(this->minField, &QLineEdit::editingFinished, this,
              &VolumeRenderingView::updateField);
      minSlider->setToolTip(
          "Sets the minimum absorption value that gets displayed.\nLower "
          "absorption values will be invisible.");

      this->maxSlider = new QSlider();
      this->maxSlider->setMinimum(0);
      this->maxSlider->setMaximum(1000);
      this->maxSlider->setOrientation(Qt::Horizontal);
      connect(this->maxSlider, &QSlider::valueChanged, this,
              &VolumeRenderingView::updateMaxSlider);
      maxLayout->addWidget(maxSlider);
      maxLayout->addWidget(
          maxField = new QLineEdit(QString::number(maxSlider->value())));
      maxField->setFixedWidth(40);
      connect(this->maxField, &QLineEdit::editingFinished, this,
              &VolumeRenderingView::updateField);
      maxSlider->setToolTip(
          "Sets the maximum absorption value that gets displayed.\nHigher "
          "absorption values will be opaque.");

      this->scaleSlider = new QSlider();
      this->scaleSlider->setMinimum(0);
      this->scaleSlider->setMaximum(5000);
      this->scaleSlider->setValue(3000);
      this->scaleSlider->setOrientation(Qt::Horizontal);
      connect(this->scaleSlider, &QSlider::valueChanged, this,
              &VolumeRenderingView::updateSlider);
      scaleLayout->addWidget(scaleSlider);
      scaleLayout->addWidget(
          scaleField = new QLineEdit(QString::number(scaleSlider->value())));
      scaleField->setFixedWidth(40);
      connect(this->scaleField, &QLineEdit::editingFinished, this,
              &VolumeRenderingView::updateField);
      scaleSlider->setToolTip(
          "All output values are scaled with this coefficient.");
    }
    sidePanel->setLayout(layout);
  }

  this->setMinimumSize(300, 200);

  connect(view3d, &vx::visualization::View3D::changed, this,
          [this] { this->update(); });
}

VolumeRenderingView::~VolumeRenderingView() {
  delete (this->randomValues);
  this->randomValues = nullptr;
}

void VolumeRenderingView::dataSetChanged() {
  auto data = this->data();
  // TODO: Will this be properly updated if the data set changes?
  this->view3d->setBoundingBox(dataSet()->boundingBox());

  double maxValue;
  if (auto dataVoxel = qSharedPointerDynamicCast<vx::VolumeDataVoxel>(data)) {
    if (CLInstance::getDefaultInstance()->isValid()) {
      dataVoxel->performInGenericContext([](auto& data3) {
        if (data3.getCLImage()() == nullptr) {
          throw vx::Exception("de.uni_stuttgart.Voxie.NoOpenCLImage",
                              "Upload of data to GPU failed");

          // TODO: handle case when a filter gets added later and the filtered
          // dataset cannot be uploaded to the GPU?
        }
      });
    }

    maxValue = dataVoxel->performInGenericContext(
        [](auto& data3) { return data3.getMinMaxValue().second; });

#ifdef VOLUME_RENDERING_VISUALIZER_CPP_ADDITIONAL_DATASETCHANGED
    VOLUME_RENDERING_VISUALIZER_CPP_ADDITIONAL_DATASETCHANGED
#endif
  } else {
    if (data)
      qWarning()
          << "Unknown volume type in VolumeRenderingView::dataSetChanged()";
    maxValue = 100;
  }

  this->maxSlider->setValue(100 > maxValue ? 100 : maxValue);
  // TODO: This should be disconnected somewhere
  connect(this->dataSet(), &VolumeNode::changed, [=] { this->update(); });
  this->updateRandomValues = true;
  this->update();
}

void VolumeRenderingView::paintEvent(QPaintEvent* event) {
  (void)event;
  QPainter painter(this);
  QString message = "";

  // this->image.fill(Qt::black );
  //--------------------------------------------------------------------------------//
  auto volumeData = this->data();
  if (auto dataVoxel =
          qSharedPointerDynamicCast<vx::VolumeDataVoxel>(volumeData)) {
    if (this->generating) {
      this->generationRequested = true;
      return;
    }

    if (!this->lightSourcesList) {
      Q_EMIT lightSourceListRequest();
    }

    QMatrix4x4 invViewProjectionMatrix = toQMatrix4x4(matrixCastNarrow<float>(
        inverse(view3d->projectionMatrix(this->image.width(),
                                         this->image.height()) *
                view3d->viewMatrix())
            .projectiveMatrix()));

    signed long quality = 32;
    if (this->radioQ1->isChecked()) quality = 64;
    if (this->radioQ2->isChecked()) quality = 128;
    if (this->radioQ3->isChecked()) quality = 256;

    auto minMax = dataVoxel->performInGenericContext(
        [](auto& data) { return data.getMinMaxValue(); });

    float scale = 0.001f * this->scaleSlider->value();
    scale *= scale;

    if (this->useAntiAliazing && this->updateRandomValues) {
      // generate container with random pixel values
      if (randomValues) {
        this->randomValues->resize(this->image.height(), this->image.width());
      } else {
        this->randomValues = new ThreadSafe_MxN_Matrix(
            this, this->image.height(), this->image.width());
      }

      this->setRandomValues(randomValues, quality);
      this->updateRandomValues = false;
    }

    if (CLInstance::getDefaultInstance()->isValid() && this->getUseGPU()) {
      try {
        const int errorCount = 22;
        cl_int err[errorCount];
        cl::Device device =
            CLInstance::getDefaultInstance()->getDevice(CL_DEVICE_TYPE_GPU);
        cl::CommandQueue queue =
            CLInstance::getDefaultInstance()->getCommandQueue(device);

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
        cl_int cl_quality = quality;

        cl_float2 voxelRange = {
            {minMax.first + (minMax.second - minMax.first) * 0.001f *
                                this->minSlider->value(),
             minMax.first + (minMax.second - minMax.first) * 0.001f *
                                this->maxSlider->value()}};

        err[0] = this->kernel.setArg<cl::Image2D>(0, this->clImage);

        dataVoxel->performInGenericContext([&err, this](auto& data) {
          err[1] = this->kernel.setArg<cl::Image3D>(1, data.getCLImage());

          if (data.getDataType() == DataType::Float16 ||
              data.getDataType() == DataType::Float32 ||
              data.getDataType() == DataType::Float64) {
            err[7] = this->kernel.setArg<cl_int>(7, 0);
          } else {
            err[7] = this->kernel.setArg<cl_int>(7, 1);
          }

          const QVector3D spacing = data.getSpacing();
          cl_float3 openClSpacing = {{spacing.x(), spacing.y(), spacing.z()}};
          err[2] = this->kernel.setArg<cl_float3>(2, (cl_float3)openClSpacing);

          const QVector3D firstVoxelPosition = data.origin();
          cl_float3 cl_firstVoxelPosition = {{firstVoxelPosition.x(),
                                              firstVoxelPosition.y(),
                                              firstVoxelPosition.z()}};
          err[9] = this->kernel.setArg<cl_float3>(9, cl_firstVoxelPosition);

          const QVector3D dimensionsMetric = data.getDimensionsMetric();
          cl_float3 cl_dimensionsMetric = {{dimensionsMetric.x(),
                                            dimensionsMetric.y(),
                                            dimensionsMetric.z()}};
          err[10] = this->kernel.setArg<cl_float3>(10, cl_dimensionsMetric);
        });

        err[3] = this->kernel.setArg<cl_float16>(3, invViewProjection);
        err[4] = this->kernel.setArg<cl_float2>(4, voxelRange);
        err[5] = this->kernel.setArg<cl_int>(5, cl_quality);
        err[6] = this->kernel.setArg<cl_float>(6, scale);
        err[8] = this->kernel.setArg<cl_bool>(
            8, (cl_int)(this->useAntiAliazing ? 1 : 0));

        Q_EMIT ambientLightRequest();

        cl_float3 ambientColor = {
            {(float)this->ambientLight.redF(),
             (float)this->ambientLight.greenF(),
             (float)this->ambientLight
                 .blueF()}};  // double values from 0.0f to 1.0f
        err[11] = this->kernel.setArg<cl_float3>(11, ambientColor);
        cl_float cl_ambientScale = this->ambientlightScale;
        err[12] = this->kernel.setArg<cl_float>(12, cl_ambientScale);
        cl_float cl_diffuseScale = this->diffuselightScale;
        err[13] = this->kernel.setArg<cl_float>(13, cl_diffuseScale);

        // conversion into an openCl compliant data structure
        cl_int newListCount = 0;

        err[14] = 0;
        err[18] = 0;
        cl::Buffer lightSourceListBuf;

        if (lightSourcesList) {
          std::vector<cl_lightSource> cl_lightSourceList;

          for (int i = 0; i < lightSourcesList->size(); i++) {
            if (lightSourcesList->at(i)->isActive()) {
              cl_lightSource newLightSource;
              newLightSource.position = {
                  {lightSourcesList->at(i)->getPosition().x(),
                   lightSourcesList->at(i)->getPosition().y(),
                   lightSourcesList->at(i)->getPosition().z(),
                   lightSourcesList->at(i)->getPosition().w()}};
              newLightSource.colorF = {
                  {(float)lightSourcesList->at(i)->getLightColor().redF(),
                   (float)lightSourcesList->at(i)->getLightColor().greenF(),
                   (float)lightSourcesList->at(i)->getLightColor().blueF(), 0}};
              newListCount++;
              cl_lightSourceList.push_back(newLightSource);
            } else {
              // qDebug()<<"lightSource"<<i<<"is not active";
            }
          }
          // conversion into openCl buffer parameter
          if (cl_lightSourceList.size() == 0) {
            lightSourceListBuf = cl::Buffer();
          } else {
            lightSourceListBuf = cl::Buffer(
                CLInstance::getDefaultInstance()->getContext(),
                CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR,
                sizeof(cl_lightSource) * cl_lightSourceList.size(),
                cl_lightSourceList.data(), &err[14]);
          }
        } else {
          qDebug() << "lightSourcesList == nullptr";
        }
        err[15] = this->kernel.setArg<cl::Buffer>(14, lightSourceListBuf);
        err[16] = this->kernel.setArg<cl_int>(15, newListCount);
        err[17] = this->kernel.setArg<cl_bool>(
            16, (this->useAbsuluteShadingValue ? 1 : 0));

        cl::Buffer randomValuesBuffer = cl::Buffer();
        if (randomValues && randomValues->size() != 0 &&
            this->randomValues->getData()) {
          randomValuesBuffer = cl::Buffer(
              CLInstance::getDefaultInstance()->getContext(),
              CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR,
              sizeof(float) * randomValues->size(),
              this->randomValues->getData(), &err[18]);
        }
        err[19] = this->kernel.setArg<cl::Buffer>(17, randomValuesBuffer);
        err[20] = this->kernel.setArg<cl_int>(18, this->image.width());

        this->generating = true;

        err[21] = queue.enqueueNDRangeKernel(
            this->kernel, cl::NullRange,
            cl::NDRange(this->image.width(), this->image.height(), 1),
            cl::NullRange);

        for (int i = 0; i < errorCount; i++)
          if (err[i] != CL_SUCCESS) {
            qDebug() << i << clErrorToString(err[i]);
            message += clErrorToString(err[i]) + "\n";
          }

        cl::size_t<3> origin, area;
        area[0] = this->image.width();
        area[1] = this->image.height();
        area[2] = 1;
        queue.enqueueReadImage(this->clImage, CL_TRUE, origin, area,
                               this->image.bytesPerLine(), 0,
                               this->image.scanLine(0));
        Q_EMIT generationDone();
      } catch (CLException& ex) {
        qWarning() << clErrorToString(ex.getErrorCode());
        message += clErrorToString(ex.getErrorCode()) + "\n";
      }

    } else {  // #OpenCL support not available. CPU implementation used instead#

      QVector2D voxelRange =
          QVector2D(minMax.first + (minMax.second - minMax.first) * 0.001f *
                                       this->minSlider->value(),
                    minMax.first + (minMax.second - minMax.first) * 0.001f *
                                       this->maxSlider->value());

      QVector3D spacing = dataVoxel->performInGenericContext(
          [](auto& data) { return data.getSpacing(); });

      Q_EMIT ambientLightRequest();
      ImageRender* imRender = new ImageRender(
          &this->image, dataVoxel, invViewProjectionMatrix, voxelRange, quality,
          scale, this->useAntiAliazing, spacing, this->ambientLight,
          this->ambientlightScale, this->diffuselightScale,
          this->lightSourcesList, this->useAbsuluteShadingValue, randomValues);

      connect(imRender, &ImageRender::generationDone, this,
              &VolumeRenderingView::reRendering);

      this->generating = true;

      imRender->render();
      delete imRender;
    }
    painter.drawImage(0, 0, this->image);
    painter.setPen(Qt::red);
    // qDebug()<<message;
    painter.drawText(QRect(8, 2, this->width(), this->height()), message);

#ifdef VOLUME_RENDERING_VISUALIZER_CPP_ADDITIONAL_PAINTEVENT
    VOLUME_RENDERING_VISUALIZER_CPP_ADDITIONAL_PAINTEVENT
#endif
  } else {
    if (volumeData)
      qWarning() << "Unknown volume type in VolumeRenderingView::paintEvent()";

    image.fill(Qt::transparent);
    painter.setPen(Qt::black);
    painter.drawText(16, 16, "DataNode not connected!");
  }
}

void VolumeRenderingView::reRendering() {
  this->generating = false;
  if (this->generationRequested) {
    this->generationRequested = false;
    this->repaint();
  }
}

void VolumeRenderingView::setRandomValues(ThreadSafe_MxN_Matrix* randomValues,
                                          signed long numSamples) {
  QThreadPool threadPool;
  for (int x = 0; x < this->image.width(); x++) {
    RandomNumberGenerationTask* randomNumberGenerationTask =
        new RandomNumberGenerationTask(x, this->image.height(), randomValues,
                                       numSamples);
    threadPool.start(randomNumberGenerationTask);
  }
  threadPool.waitForDone(-1);  // wait til all Threads are done.
}

void VolumeRenderingView::resizeEvent(QResizeEvent* event) {
  if (event->size() == event->oldSize()) {
    return;  // If this happens, something weird has happend
  }

  if (event->size().isEmpty()) {
    return;
  }

  this->image = QImage(this->width(), this->height(), QImage::Format_RGBA8888);
  this->image.fill(QColor(0, 0, 0, 0));

  if (!CLInstance::getDefaultInstance()->isValid()) {
    this->image = QImage(this->width(), this->height(), this->image.format());
  } else {
    try {
      this->clImage = CLInstance::getDefaultInstance()->createImage2D(
          cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), this->image.width(),
          this->image.height(), nullptr);
    } catch (CLException& ex) {
      qWarning() << ex;
    }
  }
  this->updateRandomValues = true;
  this->update();
}
void VolumeRenderingView::mousePressEvent(QMouseEvent* event) {
  view3d->mousePressEvent(mouseLast, event, size());
  this->mouseLast = event->pos();
}

void VolumeRenderingView::mouseMoveEvent(QMouseEvent* event) {
  view3d->mouseMoveEvent(mouseLast, event, size());
  this->mouseLast = event->pos();
}

void VolumeRenderingView::mouseReleaseEvent(QMouseEvent* event) {
  view3d->mouseReleaseEvent(mouseLast, event, size());
  this->mouseLast = event->pos();
}

void VolumeRenderingView::wheelEvent(QWheelEvent* event) {
  view3d->wheelEvent(event, size());
}

RandomNumberGenerationTask::RandomNumberGenerationTask(
    int x, int height, ThreadSafe_MxN_Matrix* randomValues,
    signed long numSamples) {
  this->x = x;
  this->height = height;
  this->randomValues = randomValues;
  this->numSamples = numSamples;
}

void RandomNumberGenerationTask::run() {
  for (int y = 0; y < this->height; y++) {
    float randomNumber = 0.0f;

    QCryptographicHash hash(QCryptographicHash::Sha1);
    uint32_t xVal = this->x;
    uint32_t yVal = y;
    hash.addData((const char*)&xVal, sizeof(xVal));
    hash.addData((const char*)&yVal, sizeof(yVal));
    auto res = hash.result();
    uint32_t random = *(uint32_t*)res.data();
    randomNumber = random / 4294967295.0;

    this->randomValues->addValue(randomNumber, this->x, y);
  }
}

// #### Visualizer ####

void VolumeRenderingVisualizer::settingUpObjectProperties() {
  objProp->setObjectName("Property Widget");
  objProp->setWindowTitle("Object Properties");
  // Properties changed by input in the GUI
  connect(objProp, &ObjectProperties::positionChanged, this,
          &VolumeRenderingVisualizer::positionEditedFromGUI);
  connect(objProp, &ObjectProperties::rotationChanged, this,
          &VolumeRenderingVisualizer::rotationEditedFromGUI);

  // GUI need the last valid Properties to reset itself
  connect(objProp, &ObjectProperties::positionRequest, [=]() {
    if (this->dataSet()) {
      objProp->setPosition(this->dataSet());
    }
  });
  connect(objProp, &ObjectProperties::rotationRequest, [=]() {
    if (this->dataSet()) {
      objProp->setRotation(this->dataSet());
    }
  });
  // Properties changed by interaction with the node
  if (this->dataSet()) {
    connect(this->mainDataset, &VolumeNode::adjustedPositionChanged, [=]() {
      if (this->dataSet()) {
        objProp->setPosition(this->dataSet());
      }
    });

    connect(this->mainDataset, &VolumeNode::adjustedRotationChanged, [=]() {
      if (this->dataSet()) {
        objProp->setRotation(this->dataSet());
      }
    });
  }
  objProp->init();
}

void VolumeRenderingVisualizer::settingUpLightSourceProperties() {
  if (this->view && lightSrcProp) {
    this->lightSrcProp->setObjectName("Property Widget");
    this->lightSrcProp->setWindowTitle("Light Source Properties");

    connect(this->view, &VolumeRenderingView::ambientLightRequest,
            this->lightSrcProp, &LightSourceProperties::ambientLightRequested);

    connect(this->lightSrcProp,
            &LightSourceProperties::ambientLightRequestResponse, this->view,
            &VolumeRenderingView::ambientLightRequestResponsed);

    connect(this->view, &VolumeRenderingView::lightSourceListRequest,
            this->lightSrcProp,
            &LightSourceProperties::lightSourcesListRequested);
    connect(this->lightSrcProp,
            &LightSourceProperties::lightSourcesListRequestResponse, this->view,
            &VolumeRenderingView::lightSourcesListRequestResponsed);

    connect(this->lightSrcProp,
            &LightSourceProperties::ambientlightScaleChanged, this->view,
            &VolumeRenderingView::ambientlightScaleChanged);

    connect(this->lightSrcProp,
            &LightSourceProperties::lightsourcesScaleChanged, this->view,
            &VolumeRenderingView::diffuselightScaleChanged);
    connect(this->lightSrcProp,
            &LightSourceProperties::useAbsShadingValueChanged, this->view,
            &VolumeRenderingView::useAbsuluteShadingValueChaned);

    connect(this->lightSrcProp, &LightSourceProperties::updateViewRequest,
            [=]() { this->view->update(); });
  }
}

void VolumeRenderingVisualizer::settingUpRenderImplementationSelection(
    RenderImplementationSelection* renderImpl) {
  renderImpl->setObjectName("Property Widget");
  renderImpl->setWindowTitle("Render Implementation Selection");

  connect(renderImpl, &RenderImplementationSelection::renderImplChanged,
          this->view, &VolumeRenderingView::setUseGPU);
  connect(renderImpl, &RenderImplementationSelection::renderImplChanged,
          this->view, [this] { this->view->repaint(); });
  connect(this, &VolumeRenderingVisualizer::noOpenClAvailable, renderImpl,
          &RenderImplementationSelection::noOpenClAvailable);

  if (!CLInstance::getDefaultInstance()->isValid()) {
    this->view->setUseGPU(false);
    Q_EMIT noOpenClAvailable();
  }
}

void VolumeRenderingView::setUseAntiAliazing(bool useAntiAliazing) {
  this->useAntiAliazing = useAntiAliazing;
}

bool VolumeRenderingView::getUseGPU() { return this->useGPU; }

void VolumeRenderingView::setUseGPU(bool useGPU) { this->useGPU = useGPU; }

void VolumeRenderingVisualizer::positionChanged(QVector3D offset) {
  auto dataSet = this->dataSet();
  if (dataSet != NULL) {
    dataSet->adjustPosition(offset);
  }
}

void VolumeRenderingVisualizer::rotationChanged(QQuaternion rotation) {
  auto dataSet = this->dataSet();
  if (dataSet != NULL) {
    dataSet->adjustRotation(rotation);
  }
}

void VolumeRenderingView::updateSlider() {
  this->minField->setText(QString::number(this->minSlider->value()));
  this->maxField->setText(QString::number(this->maxSlider->value()));
  this->scaleField->setText(QString::number(this->scaleSlider->value()));
  this->update();
}

void VolumeRenderingVisualizer::positionEditedFromGUI(QVector3D pos) {
  auto dataSet = this->dataSet();
  if (dataSet == NULL) {
    return;
  }

  dataSet->adjustPosition(pos, true);
}

void VolumeRenderingVisualizer::rotationEditedFromGUI(QQuaternion rot) {
  auto dataSet = this->dataSet();
  if (dataSet == NULL) {
    return;
  }

  dataSet->adjustRotation(rot, true);
}
// #### VolumeRenderingView public Slots ####
void VolumeRenderingView::ambientLightRequestResponsed(QColor ambientLight) {
  this->ambientLight = ambientLight;
}

void VolumeRenderingView::lightSourcesListRequestResponsed(
    QList<LightSource*>* lightSourcesList) {
  this->lightSourcesList = lightSourcesList;
}

void VolumeRenderingView::ambientlightScaleChanged(int value) {
  this->ambientlightScale = (float)value / 100.0f;
  this->update();
}

void VolumeRenderingView::diffuselightScaleChanged(int value) {
  this->diffuselightScale = (float)value / 100.0f;
  this->update();
}

void VolumeRenderingView::useAbsuluteShadingValueChaned(bool value) {
  this->useAbsuluteShadingValue = value;
  this->update();
}

void VolumeRenderingView::updateMinSlider() {
  if (minSlider->value() > maxSlider->value())
    maxSlider->setValue(minSlider->value());

  this->updateSlider();
}

void VolumeRenderingView::updateMaxSlider() {
  if (maxSlider->value() < minSlider->value())
    minSlider->setValue(maxSlider->value());

  this->updateSlider();
}

void VolumeRenderingView::updateField() {
  if (minField->text().toInt()) minSlider->setValue(minField->text().toInt());

  if (maxField->text().toInt()) maxSlider->setValue(maxField->text().toInt());

  if (scaleField->text().toInt())
    scaleSlider->setValue(scaleField->text().toInt());

  this->updateSlider();
}

void VolumeRenderingView::setMinimum(int min) {
  this->minSlider->setValue(min);
  this->updateSlider();
}

void VolumeRenderingView::setMaximum(int max) {
  this->maxSlider->setValue(max);
  this->updateSlider();
}

void VolumeRenderingView::setScale(int scale) {
  this->scaleSlider->setValue(scale);
  this->updateSlider();
}

int VolumeRenderingView::getMinimum() { return this->minSlider->value(); }

int VolumeRenderingView::getMaximum() { return this->maxSlider->value(); }

int VolumeRenderingView::getScale() { return this->scaleSlider->value(); }

NODE_PROTOTYPE_IMPL_2(VolumeRendering, Visualizer)
