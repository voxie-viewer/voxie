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

#include "SliceVisualizer.hpp"

#include <PluginVisSlice/ColorizerWorker.hpp>
#include <PluginVisSlice/ImagePaintWidget.hpp>

#include <PluginVisSlice/Ruler.hpp>
#include <PluginVisSlice/SliceAdjustmentTool.hpp>
#include <PluginVisSlice/SurfaceVisualizerTool.hpp>
#include <PluginVisSlice/ToolSelection.hpp>
#include <PluginVisSlice/ToolZoom.hpp>
#include <PluginVisSlice/VolumeGrid.hpp>

#include <PluginVisSlice/InfoWidget.hpp>
#include <PluginVisSlice/PointProperties.hpp>

#include <PluginVisSlice/Prototypes.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/VolumeNode.hpp>
#include <VoxieBackend/Data/HistogramProvider.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <Voxie/Vis/FilterChain2DWidget.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QThreadPool>

#include <QtGui/QColor>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QRgb>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>

using namespace vx::visualization;
using namespace vx;

SliceVisualizer::SliceVisualizer()
    : VisualizerNode(getPrototypeSingleton()),
      properties(new vx::SliceProperties(this)) {
  qRegisterMetaType<vx::FloatImage>();
  qRegisterMetaType<vx::SliceImage>();
  qRegisterMetaType<vx::PlaneInfo>();
  qRegisterMetaType<QVector<int>>();
  qRegisterMetaType<QVector<float>>();
  qRegisterMetaType<vx::HistogramProvider::DataPtr>();

  connect(properties, &SliceProperties::centerPointChanged, this,
          &SliceVisualizer::signalRequestSliceImageUpdate);
  connect(properties, &SliceProperties::verticalSizeChanged, this,
          &SliceVisualizer::signalRequestSliceImageUpdate);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::geometricPrimitive,
      &SliceProperties::geometricPrimitiveChanged,
      &GeometricPrimitiveNode::measurementPrimitivesChanged, this,
      &SliceVisualizer::newMeasurement);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::geometricPrimitive,
      &SliceProperties::geometricPrimitiveChanged,
      &GeometricPrimitiveNode::selectedPrimitiveChanged, this,
      &SliceVisualizer::currentPointChanged);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::geometricPrimitive,
      &SliceProperties::geometricPrimitiveChanged,
      &DataNode::dataChangedFinished, this,
      &SliceVisualizer::gpoDataChangedFinished);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::plane, &SliceProperties::planeChanged,
      &vx::PlaneNode::rotationChanged, this,
      &SliceVisualizer::rotationChangedForward);
  QObject::connect(this, &SliceVisualizer::rotationChangedForward, this,
                   [this]() {
                     // qDebug() << "&SliceVisualizer::rotationChangedForward";
                     this->emitCustomPropertyChanged(
                         this->properties->orientationProperty());
                   });

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::plane, &SliceProperties::planeChanged,
      &vx::PlaneNode::originChanged, this,
      &SliceVisualizer::originChangedForward);
  QObject::connect(
      this, &SliceVisualizer::originChangedForward, this, [this]() {
        // qDebug() << "&SliceVisualizer::originChangedForward";
        this->emitCustomPropertyChanged(this->properties->originProperty());
      });

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &DataNode::dataChangedFinished, this,
      &SliceVisualizer::volumeDataChangedFinished);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &VolumeNode::rotationChanged, this,
      &SliceVisualizer::volumeDataRotationChanged);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &VolumeNode::translationChanged, this,
      &SliceVisualizer::volumeDataTranslationChanged);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::labelContainer,
      &SliceProperties::labelContainerChanged, &DataNode::dataChangedFinished,
      this, &SliceVisualizer::labelContainerChangedFinished);

  this->view = new QWidget();
  connect(this, &Node::displayNameChanged, this->view,
          &QWidget::setWindowTitle);
  this->setAutomaticDisplayName("Slice Visualizer");

  connect(properties, &SliceProperties::volumeChanged, this,
          [this](vx::Node* value) {
            auto dataset = dynamic_cast<VolumeNode*>(value);
            if (value && !dataset) {
              qWarning() << "Cannot cast Volume property to VolumeNode";
              return;
            }

            if (dataset == this->mainVolumeNode) return;

            if (this->mainVolumeNode) {
              this->disconnect(this->mainVolumeNode, 0, this, 0);
              this->disconnect(this->_slice, 0, this, 0);
              this->setAutomaticDisplayName("Slice Visualizer");
              this->mainVolumeNode = nullptr;
              this->_slice = nullptr;
              updateCreatePlaneNode();
              this->redraw();
              Q_EMIT this->signalRequestSliceImageUpdate();
            }
            if (dataset) {
              this->mainVolumeNode = dataset;
              this->_slice = new Slice(dataset);
              updateCreatePlaneNode();
              if (this->planeProperty) {
                this->slice()->setPlane(*planeProperty->plane().data());
              }
              this->initializeSV();
            }
          });

  connect(properties, &SliceProperties::planeChanged, this,
          [this](vx::Node* value) {
            auto plane = dynamic_cast<PlaneNode*>(value);
            if (value && !plane) {
              qWarning() << "Cannot cast Plane property to PlaneNode";
              return;
            }
            if (plane == this->planeProperty) return;

            if (plane) {
              this->planeProperty = plane;
              if (this->slice())
                this->slice()->setPlane(*planeProperty->plane().data());

              connect(this->planeProperty, &PlaneNode::rotationChanged, this,
                      [this](QQuaternion rotation) {
                        propagateToPlaneNode = false;
                        this->setRotation(rotation);
                        propagateToPlaneNode = true;
                      });
              connect(this->planeProperty, &PlaneNode::originChanged, this,
                      [this](QVector3D origin) {
                        propagateToPlaneNode = false;
                        this->setOrigin(origin);
                        propagateToPlaneNode = true;
                      });

            } else {
              // TODO: Don't use broad disconnect() calls
              disconnect(this->planeProperty, 0, this, 0);
              this->planeProperty = nullptr;
            }
            updateCreatePlaneNode();
          });

  // Update image when interpolation is changed
  connect(properties, &SliceProperties::interpolationChanged, this, [this]() {
    doGenerateSliceImage(this->_slice, currentPlaneArea(), canvasSize(),
                         getInterpolation(this->properties));
  });

  this->view->setMinimumSize(300, 200);

  this->_imageDisplayingWidget =
      new ImagePaintWidget(this);  // has dependencies in tools
  this->view->setFocusProxy(
      this->_imageDisplayingWidget);  // <- receives all keyboard events

  QSizePolicy pol(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  pol.setHorizontalStretch(0);
  pol.setVerticalStretch(0);
  this->_imageDisplayingWidget->setSizePolicy(pol);

  imageLayer = ImageLayer::create(this);
  imageLayer->setObjectName(this->imageLayerName);
  this->layers_.append(imageLayer);

  this->sliceAdjustmentTool = new SliceAdjustmentTool(view, this);
  this->sliceAdjustmentTool->setObjectName("SliceAdjustmentTool");
  this->_tools.append(this->sliceAdjustmentTool);
  auto sal = SliceAdjustmentLayer::create(this);
  this->layers_.append(sal);

  ToolZoom* tz = new ToolZoom(view, this);
  tz->setObjectName("ToolZoom");
  this->_tools.append(tz);

  auto sv = SurfaceVisualizerTool::create(this);
  sv->setObjectName("SurfaceVisualizerTool");
  this->layers_.append(sv);  // only elements in layers_ will be visible over
                             // slice

  GeometricAnalysisTool* gat = new GeometricAnalysisTool(view, this);
  gat->setObjectName("GeometricAnalysisTool");
  this->_tools.append(gat);
  auto gal = GeometricAnalysisLayer::create(this);
  this->layers_.append(gal);

  this->selectionTool = new ToolSelection(view, this);
  this->selectionTool->setObjectName("ToolSelection");
  this->_tools.append(this->selectionTool);
  this->layers_.append(this->selectionTool->layer());

  // *** Automatically sets the elements of _tools into the tool view ***
  if (_tools.size() <= 0) {
    qWarning() << "No tool found in slicevisualizer";
  } else {
    toolBar = new QWidget(view);
    QHBoxLayout* l = new QHBoxLayout();
    toolBar->setLayout(l);
    l->setAlignment(Qt::AlignLeft);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    l->setMargin(0);

    Visualizer2DTool** data = _tools.data();
    for (int i = 0; i < _tools.size(); ++i) {
      this->toolBar->layout()->addWidget(data[i]);
    }
  }
  // ***Automatically sets of tools view END***

  //*********FilterChain2D***********
  _filterChain2DWidget = new FilterChain2DWidget(view);

  this->dynamicSections().append(this->_filterChain2DWidget);
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::allFiltersApplied, [=]() {
            this->onSliceImageFiltered(
                this->_filterChain2DWidget->getFilterChain()->getOutputSlice());
          });
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::allFiltersApplied, [=]() {
            this->_filterWorkerRunning = false;
            runFilterWorker();
          });
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::filterListChanged, this,
          &SliceVisualizer::applyFilters);
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::filterChanged, this,
          &SliceVisualizer::applyFilters);
  connect(this->_filterChain2DWidget,
          &FilterChain2DWidget::requestFilterMaskEditor, this,
          &SliceVisualizer::onFilterMaskRequest);
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::filterListChanged, this, [this]() {
            this->emitCustomPropertyChanged(
                this->properties->filter2DConfigurationProperty());
          });
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::filterChanged, this, [this]() {
            this->emitCustomPropertyChanged(
                this->properties->filter2DConfigurationProperty());
          });

  //*** COLORIZER ****
  // TODO: This connection should probably be made in HistogramWidget
  connect(properties, &SliceProperties::valueColorMappingChanged, this,
          [=](const QList<vx::ColorizerEntry>& value) {
            // after any change on the standard mapping
            // send over the new color mappings to the histogram widget and
            // update it
            QSharedPointer<Colorizer> colorizer =
                QSharedPointer<Colorizer>(new Colorizer());
            colorizer->setEntries(value);
            _histogramWidget->setColorizer(colorizer);
          });

  //**** INFOWIDGET ***
  infoWidget = new InfoWidget(this);
  connect(this, &QObject::destroyed, infoWidget, &QObject::deleteLater);

  //**** HISTOGRAM PROVIDER ****
  histogramProvider = QSharedPointer<vx::HistogramProvider>::create();

  connect(this, &SliceVisualizer::volumeDataChangedFinished, [this]() {
    disconnect(histogramConnection);

    auto volume = dynamic_cast<VolumeNode*>(properties->volume());
    auto data = volume ? volume->data() : QSharedPointer<Data>();

    if (auto volumeDataVoxel =
            qSharedPointerDynamicCast<VolumeDataVoxel>(data)) {
      auto volumeHistogramProvider = volumeDataVoxel->getHistogramProvider(
          HistogramProvider::DefaultBucketCount);

      histogramConnection = connect(
          volumeHistogramProvider.data(), &HistogramProvider::dataChanged,
          histogramProvider.data(), &HistogramProvider::setData);

      histogramProvider->setData(volumeHistogramProvider->getData());
    }
  });

  //**** SLICE HISTOGRAM PROVIDER ****
  sliceHistogramProvider = QSharedPointer<vx::HistogramProvider>::create();
  // TODO: Should the histogram calculation happen in a background thread?
  // This probably would require making sure that there are not multiple updates
  // at the same time and the the last result is used etc.
  // TODO: Currently this histogram is updated even if it is not used. Avoid
  // this?
  connect(this, &SliceVisualizer::signalRequestHistogram, this,
          [this](vx::SliceImage& image) {
            sliceHistogramProvider->setDataFromFloatImage(
                image, HistogramProvider::DefaultBucketCount);
          });

  //**** HISTOGRAMWIDGET ***
  histogramBox = new QWidget();
  QVBoxLayout* histogramLayout = new QVBoxLayout(histogramBox);
  QHBoxLayout* radioButtonLayout = new QHBoxLayout();
  connect(this, &QObject::destroyed, histogramBox, &QObject::deleteLater);
  volumeRadioButton = new QRadioButton("Volume");
  volumeRadioButton->setChecked(true);
  connect(volumeRadioButton, &QRadioButton::toggled, [this](bool checked) {
    // as long as we only have two radiobuttons it is enough to check if one of
    // the two is being checked/unchecked
    if (checked)
      _histogramWidget->setHistogramProvider(histogramProvider);
    else
      _histogramWidget->setHistogramProvider(sliceHistogramProvider);
  });
  radioButtonLayout->addWidget(volumeRadioButton);
  sliceRadioButton = new QRadioButton("Slice");
  radioButtonLayout->addWidget(sliceRadioButton);
  _histogramWidget = new HistogramWidget();
  histogramLayout->addLayout(radioButtonLayout);
  histogramLayout->addWidget(_histogramWidget);

  _histogramWidget->setHistogramProvider(histogramProvider);

  //**** GEOMETRICANALYSIS ***
  PointProperties* _pointListWidget = new PointProperties(view, this);
  QString name = "Geometric Analysis";
  _pointListWidget->setWindowTitle(name);
  this->dynamicSections().append(_pointListWidget);
  connect(_pointListWidget, &PointProperties::newVisibility, gal.data(),
          &GeometricAnalysisLayer::newVisibility);

  //**** Grid ***
  auto grid = Grid::create(this);
  this->layers_.append(grid);

  //**** RULERWIDGET ***
  auto ruler = Ruler::create(this);
  this->layers_.append(ruler);

  //**** RULERWIDGET *** END

  //**** VolumeGridWidget ***
  auto volumegrid = VolumeGrid::create(this);
  this->layers_.append(volumegrid);

  //**** VolumeGridWidget *** END

  //*****   ****

  hobox = new QVBoxLayout(view);
  hobox->setSpacing(0);
  this->view->setLayout(hobox);

  hobox->addWidget(_imageDisplayingWidget);
  hobox->addWidget(toolBar);
  connect(this->_imageDisplayingWidget, &ImagePaintWidget::resized, this,
          &SliceVisualizer::onCanvasResized);

  _resizeTimer.setSingleShot(true);
  connect(&this->_resizeTimer, &QTimer::timeout, this,
          &SliceVisualizer::resized);
  connect(this, &SliceVisualizer::resized, this,
          &SliceVisualizer::signalRequestSliceImageUpdate);

  // TODO: This often will be triggered several times for one changed, avoid
  // recomputing the slice several times in this case
  connect(this, &SliceVisualizer::signalRequestSliceImageUpdate, this,
          &SliceVisualizer::updateSliceImageFromSlice);

  if (_tools.size() > 0) {
    this->switchToolTo(this->currentTool());
  }

  this->view->show();

  _imageDisplayingWidget->setFocus();
  _imageDisplayingWidget->show();
  toolBar->show();

  createPlaneNodeAction = new QAction("Create plane property node", this);
  QObject::connect(createPlaneNodeAction, &QAction::triggered, this,
                   &SliceVisualizer::createPlaneNode);
  updateCreatePlaneNode();
  addContextMenuAction(createPlaneNodeAction);

  addSegmentationFunctionality(10);
}

QWidget* SliceVisualizer::getCustomPropertySectionContent(const QString& name) {
  if (name == "de.uni_stuttgart.Voxie.SliceVisualizer.Histogram") {
    return histogramBox;
  } else if (name == "de.uni_stuttgart.Voxie.SliceVisualizer.Info") {
    return infoWidget;
  } else {
    return Node::getCustomPropertySectionContent(name);
  }
}

void SliceVisualizer::initializeSV() {
  resetPlaneArea();

  // Slice update
  // This has to be before the Slice::planeChanged connections below to make
  // sure the slice has been updated before the redraw is started. TODO: clean
  // this up to avoid dependencies on the order of the connections.
  connect(this->_slice, &Slice::planeChanged, this,
          &SliceVisualizer::onSliceChanged);
  connect(this->_slice->getDataset(), &VolumeNode::changed, this,
          &SliceVisualizer::onDatasetChanged);

  if (mainVolumeNode != nullptr) {
    this->setAutomaticDisplayName(this->dataSet()->displayName());

    connect(this->mainVolumeNode, &Node::displayNameChanged, this, [this] {
      this->setAutomaticDisplayName(this->dataSet()->displayName());
    });
  }

  for (const auto& layer : this->layers()) {
    connect(this, &SliceVisualizer::resized, layer.data(),
            [this, layer]() { layer->onResize(this->canvasSize()); });
    Layer* layerPtr = layer.data();
    connect(layerPtr, &Layer::resultImageChanged, this,
            [this, layerPtr](const QImage& image) {
              this->addToDrawStack(layerPtr, image);
              this->redraw();
            });
    connect(
        layerPtr, &Layer::getRenderingParameters, this,
        [this](QSharedPointer<vx::ParameterCopy>& parameters, QSize& size) {
          if (QThread::currentThread() != this->thread()) {
            qCritical() << "Attempting to call getRenderingParameters() "
                           "from wrong thread";
            return;
          }
          size = this->canvasSize();
          parameters = ParameterCopy::getParameters(this);
        },
        Qt::DirectConnection);
    layer->triggerRedraw();  // Make sure all layers are drawn at least once
  }

  connect(this->_slice, &Slice::planeChanged,
          this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::onPlaneChanged, Qt::DirectConnection);

  if (mainVolumeNode) {
    //_colorizerWidget->setVolumeNode(this->dataSet());  // TODO

    connect(this->_slice->getDataset(), &VolumeNode::changed, this,
            &SliceVisualizer::onDatasetChanged);

    // TODO
    /*
    this->initializeWorker = new InitializeColorizeWorker(dataSet());
    this->initializeWorker->setAutoDelete(false);
    connect(initializeWorker, &InitializeColorizeWorker::init, this,
            &SliceVisualizer::initializeFinished);
    connect(
        this->_colorizerWidget, &SliceImageColorizerWidget::startInitProcess,
        [this]() { QThreadPool::globalInstance()->start(initializeWorker); });
    */
  }

  Q_EMIT this->signalRequestSliceImageUpdate();
}

double SliceVisualizer::getCurrentPixelSize(SlicePropertiesBase* properties,
                                            const QSize& canvasSize) {
  int heightC = canvasSize.height();
  double heightF = properties->verticalSize();
  return heightF / (double)heightC;
}

QRectF SliceVisualizer::getCurrentPlaneArea(SlicePropertiesBase* properties,
                                            const QSize& canvasSize) {
  int widthC = canvasSize.width();
  int heightC = canvasSize.height();
  float aspectRatio = (float)widthC / heightC;
  float heightF = properties->verticalSize();
  float widthF = heightF * aspectRatio;
  auto center = properties->centerPoint();
  return QRectF(center.x() - widthF / 2, center.y() - heightF / 2, widthF,
                heightF);
}
QRectF SliceVisualizer::currentPlaneArea() {
  return getCurrentPlaneArea(this->properties, this->canvasSize());
}

QPointF SliceVisualizer::planePointToPixel(const QSize& imageSize,
                                           const QRectF& planeArea,
                                           const QPointF& planePoint) {
  QPoint pixel;
  pixel.setY(-(planePoint.y() - planeArea.bottom()) * imageSize.height() /
             (planeArea.height()));
  pixel.setX((planePoint.x() - planeArea.left()) * imageSize.width() /
             (planeArea.width()));
  return pixel;
}

void SliceVisualizer::switchToolTo(Visualizer2DTool* tool) {
  Visualizer2DTool** data = _tools.data();
  int i;
  for (i = 0; i < _tools.size(); ++i) {
    if (data[i] == tool) {
      break;
    }
  }
  if (i == _tools.size()) {
    return;
  }
  if (this->_currentTool != i) data[_currentTool]->deactivateTool();
  this->_currentTool = i;
  this->emitCustomPropertyChanged(this->properties->showSliceCenterProperty());
  data[_currentTool]->activateTool();
  this->_imageDisplayingWidget->setFocus();
}

void SliceVisualizer::updateSliceImageFromSlice() {
  if (slice() == nullptr) {
    return;
  }

  int width = this->canvasWidth();
  int height = this->canvasHeight();
  if (width > 0 && height > 0 && currentPlaneArea().height() > 0 &&
      currentPlaneArea().width() > 0) {
    doGenerateSliceImage(this->_slice, currentPlaneArea(), QSize(width, height),
                         getInterpolation(this->properties));
  }
}

void SliceVisualizer::doGenerateSliceImage(
    vx::Slice* slice, const QRectF& sliceArea, const QSize& imageSize,
    vx::InterpolationMethod interpolation) {
  if (slice == nullptr) {
    return;
  }
  if (_imageQueueWorker != nullptr) {
    delete _imageQueueWorker;
  }

  if (mainVolumeNode) {
    auto plane = slice->getCuttingPlane();
    auto adjustedRotation = this->dataSet()->getAdjustedRotation();
    auto adjustedPosition = this->dataSet()->getAdjustedPosition();
    plane.origin =
        adjustedRotation.inverted() * (plane.origin - adjustedPosition);
    plane.rotation = (adjustedRotation.inverted() * plane.rotation);
    _imageQueueWorker =
        new ImageGeneratorWorker(slice->getDataset()->volumeData(), plane,
                                 sliceArea, imageSize, interpolation);
    if (!_imageWorkerRunning) {
      runSliceImageGeneratorWorker();
    }
  }
}

QVariant SliceVisualizer::getNodePropertyCustom(QString key) {
  if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration") {
    return this->_filterChain2DWidget->getFilterChain()->toXMLString();
  } else if (key ==
             "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation") {
    // TODO: This probably should return something even if no volume is
    // connected
    if (this->_slice)
      return QVariant::fromValue(
          PropertyValueConvertRaw<vx::TupleVector<double, 4>, QQuaternion>::
              toRaw(this->_slice->getCuttingPlane().rotation));
    else
      return QVariant::fromValue<vx::TupleVector<double, 4>>(
          vx::TupleVector<double, 4>(1, 0, 0, 0));
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin") {
    // TODO: This probably should return something even if no volume is
    // connected
    if (this->_slice)
      return QVariant::fromValue(
          PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::toRaw(
              this->_slice->getCuttingPlane().origin));
    else
      return QVariant::fromValue<vx::TupleVector<double, 3>>(
          vx::TupleVector<double, 3>(0, 0, 0));
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter") {
    // TODO: remove the concept of "current tool" and make this a normal
    // property
    return QVariant::fromValue<bool>(
        dynamic_cast<SliceAdjustmentTool*>(this->currentTool()));
  } else {
    return VisualizerNode::getNodePropertyCustom(key);
  }
}
void SliceVisualizer::setNodePropertyCustom(QString key, QVariant value) {
  if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration") {
    this->_filterChain2DWidget->getFilterChain()->fromXMLString(
        Node::parseVariant<QString>(value));
  } else if (key ==
             "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation") {
    // Note: If there is a plane, setting the origin / orientation is ignored
    // TODO: This probably should do something even if no volume is connected
    if (this->_slice && !properties->plane())
      this->_slice->setRotation(
          PropertyValueConvertRaw<vx::TupleVector<double, 4>, QQuaternion>::
              fromRaw(Node::parseVariant<vx::TupleVector<double, 4>>(value)));
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin") {
    // Note: If there is a plane, setting the origin / orientation is ignored
    // TODO: This probably should do something even if no volume is connected
    if (this->_slice && !properties->plane())
      this->_slice->setOrigin(
          PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::
              fromRaw(Node::parseVariant<vx::TupleVector<double, 3>>(value)));
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter") {
    // TODO: remove the concept of "current tool" and make this a normal
    // property
    auto val = Node::parseVariant<bool>(value);
    bool cval = dynamic_cast<SliceAdjustmentTool*>(currentTool());
    if (val == cval) return;
    for (const auto& tool : tools()) {
      bool isSliceAdjustment = dynamic_cast<SliceAdjustmentTool*>(tool);
      if ((val && isSliceAdjustment) || (!val && !isSliceAdjustment)) {
        this->switchToolTo(tool);
        return;
      }
    }
  } else {
    VisualizerNode::setNodePropertyCustom(key, value);
  }
}

QSharedPointer<QObject> SliceVisualizer::getPropertyUIData(
    QString propertyName) {
  if (propertyName ==
      "de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

void SliceVisualizer::runSliceImageGeneratorWorker() {
  if (!_imageQueueWorker || _imageWorkerRunning) {
    return;
  }
  ImageGeneratorWorker* worker = _imageQueueWorker;
  _imageQueueWorker = nullptr;
  _imageWorkerRunning = true;
  connect(worker, &ImageGeneratorWorker::imageGenerated, this,
          &SliceVisualizer::onSliceImageGenerated);
  worker->setAutoDelete(true);
  connect(worker, &ImageGeneratorWorker::imageGenerated, this, [=]() -> void {
    _imageWorkerRunning = false;
    runSliceImageGeneratorWorker();  // run next item in queue if existant
  });
  QThreadPool::globalInstance()->start(worker);
}

void SliceVisualizer::onCanvasResized(QResizeEvent* event) {
  Q_UNUSED(event);
  this->_resizeTimer.stop();
  this->_resizeTimer.start(RESIZE_TIMER_TIMEOUT);
}

void SliceVisualizer::initializeFinished(QVector<float> values) {
  // TODO
  (void)values;
  /*
  this->clearMappings();
  qDebug() << "initFinisched Color" + QString::number(values.at(0)) + " _ " +
                  QString::number(values.at(1));
  this->addColorMapping(values.at(0), qRgba(0, 0, 0, 255));
  this->addColorMapping(values.at(1), qRgba(255, 255, 255, 255));
  */
}

void SliceVisualizer::createPlaneNode() {
  if (this->slice()) {
    if (this->properties->plane()) {
      qWarning() << "SliceVisualizer::createPlaneNode(): SliceVisualizer "
                    "already has a plane";
      return;
    }
    auto plane = this->slice()->getPlane();
    auto planeProperty = createNode<PlaneNode>({
        {Prop::Orientation, plane.rotation},
        {Prop::Origin, plane.origin},
    });
    this->properties->setPlane(planeProperty.data());
  }
}

void SliceVisualizer::updateCreatePlaneNode() {
  createPlaneNodeAction->setEnabled(this->slice() && !planeProperty);
}

void SliceVisualizer::redraw() { _imageDisplayingWidget->update(); }

void SliceVisualizer::applyFilters() {
  this->_filterQueueImage = this->_sliceImage.clone();
  this->_filterImageWorked = false;
  // apply to clone so sliceImage will not be modified
  if (!_filterWorkerRunning) {
    runFilterWorker();
  }
}

void SliceVisualizer::runFilterWorker() {
  if (this->slice() == nullptr) {
    return;
  }
  if (_filterWorkerRunning || _filterImageWorked) {
    return;
  }
  _filterWorkerRunning = true;
  this->_filterImageWorked = true;
  this->_filterChain2DWidget->applyFilter(this->_filterQueueImage);
}

void SliceVisualizer::onSliceChanged(const vx::Slice* slice,
                                     const vx::PlaneInfo& oldPlane,
                                     const vx::PlaneInfo& newPlane,
                                     bool equivalent) {
  Q_UNUSED(slice);
  if (equivalent && oldPlane.origin != newPlane.origin) {
    QPointF difference = newPlane.get2DPlanePoint(oldPlane.origin);
    this->properties->setCenterPoint(this->properties->centerPoint() +
                                     difference);
  }

  if (this->propagateToPlaneNode) {
    this->originChanged(newPlane.origin);
    this->rotationChanged(newPlane.rotation);
  }
  Q_EMIT signalRequestSliceImageUpdate();
}

void SliceVisualizer::onDatasetChanged() {
  Q_EMIT signalRequestSliceImageUpdate();
}

void SliceVisualizer::onSliceImageGenerated(SliceImage image) {
  this->_sliceImage = image;
  this->applyFilters();
}

void SliceVisualizer::onSliceImageFiltered(SliceImage image) {
  this->_filteredSliceImage = image;
  // TODO: Currently the base image is extracted twice, once for the histogram
  // (here) and once for showing (in ImageLayer). It should be extracted only
  // once, ore the histogram should be switched to a volume histogram.
  Q_EMIT this->signalRequestHistogram(this->_filteredSliceImage);
}

void SliceVisualizer::onFilterMaskRequest(vx::filter::Filter2D* filter) {
  this->selectionTool->setMask(filter);
  this->switchToolTo(this->selectionTool);
}

QSize SliceVisualizer::canvasSize() {
  return this->_imageDisplayingWidget->size();
}

int SliceVisualizer::getLayerIndexByName(QString name) {
  int layerIndex = 0;
  for (int i = 0; i < this->layers().size(); i++) {
    if (this->layers()[0]->objectName().compare(name)) layerIndex = i;
  }
  return layerIndex;
}

void SliceVisualizer::addSegmentationFunctionality(quint8 brushRadius) {
  // get image layer index
  int imageLayerIdx = getLayerIndexByName(this->imageLayerName);

  // selection Layer
  auto selectionLayer = SelectionLayer::create(this);
  this->layers_.insert(imageLayerIdx + 1, selectionLayer);

  // labelLayer
  auto labelLayer = LabelLayer::create(this);
  this->layers_.insert(imageLayerIdx + 2, labelLayer);

  // Brush Layer
  auto brushLayer = BrushSelectionLayer::create(this);
  brushLayer->setBrushRadius(brushRadius);
  brushLayer->setObjectName(this->brushLayerName);
  this->layers_.insert(imageLayerIdx + 3, brushLayer);
  this->brushSelectionLayer = brushLayer.data();

  // Brush Tool
  BrushSelectionTool* brush = new BrushSelectionTool(view, this);
  brush->setBrushRadius(brushRadius);
  brush->setObjectName(this->brushSelectionName);
  this->brushSelectionTool = brush;
  this->_tools.append(brush);

  // Lasso Layer
  auto lassoLayer = LassoSelectionLayer::create(this);
  lassoLayer->setObjectName(this->lassoLayerName);
  this->layers_.insert(imageLayerIdx + 4, lassoLayer);
  this->lassoSelectionLayer = lassoLayer.data();

  // Lasso Tool
  auto* lasso = new LassoSelectionTool(view, this);
  lasso->setObjectName(this->lassoSelectionName);
  this->lassoSelectionTool = lasso;
  this->_tools.append(lasso);
}

void SliceVisualizer::activateBrushSelectionTool() {
  if (this->brushSelectionTool) switchToolTo(this->brushSelectionTool);
}

void SliceVisualizer::deactivateBrushSelectionTool() {
  if (this->currentTool()->objectName() == this->brushSelectionName) {
    if (this->sliceAdjustmentTool) {
      switchToolTo(this->sliceAdjustmentTool);
    }
  }
}

void SliceVisualizer::activateLassoSelectionTool() {
  if (this->lassoSelectionTool) switchToolTo(this->lassoSelectionTool);
}

void SliceVisualizer::deactivateLassoSelectionTool() {
  if (this->currentTool()->objectName() == this->lassoSelectionName) {
    if (this->sliceAdjustmentTool) {
      switchToolTo(this->sliceAdjustmentTool);
    }
  }
}

void SliceVisualizer::setBrushRadius(quint8 radius) {
  if (this->brushSelectionLayer)
    this->brushSelectionLayer->setBrushRadius(radius);

  if (this->brushSelectionTool)
    this->brushSelectionTool->setBrushRadius(radius);
}

quint8 SliceVisualizer::getBrushRadius() {
  if (this->brushSelectionTool)
    return this->brushSelectionTool->getBrushRadius();

  return 0;
}

QSharedPointer<vx::HistogramProvider>
SliceVisualizer::getSliceHistogramProvider() {
  return sliceHistogramProvider;
}

void SliceVisualizer::setHistogramColorizer(
    QSharedPointer<vx::Colorizer> colorizer) {
  _histogramWidget->setColorizer(colorizer);

  try {
    auto value = colorizer->getEntriesAsQList();
    properties->setValueColorMapping(value);

  } catch (Exception& e) {
    qCritical() << "SliceVisualizer::getSliceHistogramProvider Error while "
                   "updating property value:"
                << e.what();
  }
}

void SliceVisualizer::setVolume(vx::Node* volume) {
  this->properties->setVolume(volume);
}

QSharedPointer<vx::Colorizer> SliceVisualizer::getHistogramColorizer() {
  return _histogramWidget->colorizer();
}

QRectF SliceVisualizer::adjustedAreaAspect(const QRectF& area,
                                           const QSize& oldSize,
                                           const QSize& newSize) {
  QRectF newArea;
  if (oldSize.width() < 1 || oldSize.height() < 1) {
    if (newSize.width() < 1 || newSize.height() < 1) {
      return area;
    } else {
      newArea = area;
    }
  } else {
    qreal deltaW =
        (newSize.width() - oldSize.width()) / (oldSize.width() * 1.0);
    qreal deltaH =
        (newSize.height() - oldSize.height()) / (oldSize.height() * 1.0);
    deltaW *= area.width();
    deltaH *= area.height();
    newArea = QRectF(area.left(), area.top(), area.width() + deltaW,
                     area.height() + deltaH);
  }
  // adjust aspect
  qreal areaAspect = newArea.width() / newArea.height();
  qreal imageAspect = (newSize.width() * 1.0) / newSize.height();
  qreal imageAspectInv = (newSize.height() * 1.0) / newSize.width();

  if (fabs(areaAspect - imageAspect) >
      1e-5) {  // same as areaAspect != imageAspect
    if (imageAspect > areaAspect) {
      // image wider than area, lets add to area's width
      qreal width = newArea.height() * imageAspect;
      // let area grow left and right, so adjust x position
      qreal x = newArea.x();

      return QRectF(x, newArea.y(), width, newArea.height());
    } else {
      // image taller than area, lets add to area's height
      qreal height = newArea.width() * imageAspectInv;
      // let area grow top and bottom, so adjust y position
      qreal y = newArea.y();

      return QRectF(newArea.x(), y, newArea.width(), height);
    }
  } else {
    return newArea;
  }
}

QRectF SliceVisualizer::zoomedArea(const QRectF& area, qreal zoom) {
  if (zoom == 0) {
    return area;
  }
  qreal width = area.width() * zoom;
  qreal height = area.height() * zoom;
  // zoom to center
  qreal x = area.x() - ((width - area.width()) / 2);
  qreal y = area.y() - ((height - area.height()) / 2);
  return QRectF(x, y, width, height);
}

vx::InterpolationMethod SliceVisualizer::getInterpolation(
    vx::SlicePropertiesBase* properties) {
  return getInterpolationFromString(properties->interpolation());
}

void SliceVisualizer::renderEverything(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters) {
  imageLayer->render(outputImage, parameters);

  // TODO: when renderEverything() is called from a background thread, is it
  // okay to access layers()? => Make a copy of layers()
  for (const auto& tool : this->layers()) {
    tool->render(outputImage, parameters);
  }
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
SliceVisualizer::getRenderFunction() {
  return [this](const QSharedPointer<vx::ImageDataPixel>& outputImage,
                const vx::VectorSizeT2& outputRegionStart,
                const vx::VectorSizeT2& size,
                const QSharedPointer<vx::ParameterCopy>& parameters,
                const QSharedPointer<vx::VisualizerRenderOptions>& options) {
    // TODO: This lambda should not capture this, but currently this is
    // needed because tools is accessed (and the tools will be destroyed
    // when the SliceVisualizer is destroyed).

    Q_UNUSED(options);

    quint64 width = size.x;
    quint64 height = size.y;

    if (!outputImage)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Output image is nullptr");
    if (outputRegionStart.x > outputImage->width() ||
        (outputRegionStart.x + width) > outputImage->width() ||
        (outputRegionStart.x + width) < width ||
        outputRegionStart.y > outputImage->height() ||
        (outputRegionStart.y + height) > outputImage->height() ||
        (outputRegionStart.y + height) < height)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Region is outsize image");

    QImage qimage(width, height, QImage::Format_ARGB32);
    qimage.fill(qRgba(0, 0, 0, 0));  // Fill will transparent // TODO:
                                     // doesn't really work currently
    renderEverything(qimage, parameters);
    outputImage->fromQImage(qimage, outputRegionStart);
  };
}

void SliceVisualizer::addToDrawStack(Layer* self, const QImage& image) {
  int idx;
  for (idx = 0; idx < layers().size(); idx++) {
    if (layers().at(idx) == self) {
      break;
    }
  }
  if (idx == layers().size()) {
    qWarning() << "SliceVisualizer::addToDrawStack(): Cannot find layer"
               << self;
    qDebug() << "===";
    for (const auto& layer : this->layers()) qDebug() << "Layer:" << layer;
    qDebug() << "===";
    return;
  }
  _drawStack.remove(idx);
  _drawStack.insert(idx, image);
}

// TODO: clean up the plane change notification code

void SliceVisualizer::rotationChanged(QQuaternion rotation) {
  if (this->planeProperty) {
    this->planeProperty->setRotation(rotation);
  } else {
    // When there is a slice connected, the rotationChangedForward signal will
    // already be triggered by setRotation()
    this->emitCustomPropertyChanged(this->properties->orientationProperty());
  }
}

void SliceVisualizer::originChanged(QVector3D origin) {
  if (this->planeProperty) {
    this->planeProperty->setOrigin(origin);
  } else {
    // When there is a slice connected, the originChangedForward signal will
    // already be triggered by setOrigin()
    this->emitCustomPropertyChanged(this->properties->originProperty());
  }
}

bool SliceVisualizer::isAllowedParent(vx::NodeKind object) {
  return object == NodeKind::Data || object == NodeKind::Property ||
         object == NodeKind::Filter;
}

NODE_PROTOTYPE_IMPL_2(Slice, Visualizer)
